
#include "CSession.h"
#include "../common/Const.h"
#include "../message/MsgNode.h"
#include "../server/CServer.h"

#include "../../infra/log/Logger.h"
#include "../../services/CommunicationService/ClientInfo.h"
#include "../../services/CommunicationService/ClientManager.h"

CSession::CSession(boost::asio::io_context &ioc, boost::asio::ip::tcp::socket socket, CServer *server)
    : _ioc(ioc),
      _server(server),
      _socket(std::move(socket)), // 使用移动语义
      _bStop(false),
      _clientInfo(nullptr)
{
    // 随机生成 uuid
    auto uuid = boost::uuids::random_generator()();
    this->_uuid = boost::uuids::to_string(uuid);
    // 初始化消息节点
    this->_recvNode = std::make_shared<MsgNode>();
}

CSession::~CSession()
{
    try
    {
        LOG_INFO << " CSession is destructed." << std::endl;

        // 直接关闭 Socket，不要调用 Close()
        // 因为 Close() 中含有 shared_from_this()，在析构中调用必死
        if (_socket.is_open())
        {
            boost::system::error_code ec;
            _socket.close(ec);
        }

        // 2. 清理 ClientManager (加固)
        // 先把 shared_ptr 复制一份到本地，防止多线程竞争下 _clientInfo 突然变空
        auto info = _clientInfo;
        if (info)
        {
            // 再次检查名字是否为空
            std::string name = info->GetName();
            if (!name.empty())
            {
                LOG_INFO << "Auto removing client: " << name << std::endl;
                // 确保 ClientManager 还是活着的 (虽然它是静态单例，理论上一直活着)
                ClientManager::GetInstance().RemoveClient(name);
            }
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Destructor error: " << e.what() << '\n';
        std::cerr << "Destructor Exception: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Destructor Unknown Exception" << std::endl;
    }
}

void CSession::Close()
{
    // 以原子操作更改状态
    if (this->_bStop.exchange(true))
        return;

    // 将传入的 lambda 任务提交到 io_context 的任务队列中，等待 io_context 对应的 IO 线程去执行
    boost::asio::post(_ioc, [self = shared_from_this()]()
                      {
        boost::system::error_code ec;
        self->_socket.close(ec); });
}

void CSession::Send(const MessageHeader &header, const char *body, const uint32_t &len)
{
    if (_bStop)
        return;

    // 构造 MsgNode（在调用线程完成，避免阻塞 IO 线程）
    // 注意传入的数量 ！！！
    auto node = std::make_shared<MsgNode>(len);

    // 拷贝 Header
    node->GetHeader() = header;
    // 重新获取长度
    node->GetHeader().length = len;

    // 拷贝 Body
    if (len > 0 && body != nullptr)
    {
        std::memcpy(node->GetBody(), body, len);
    }

    // 转网络字节序（只在发送前做一次）
    node->GetHeader().ToNetwork();

    // 构建连续发送缓冲区
    node->BuildSendBuffer();

    auto self = shared_from_this();

    // LOG_INFO << "Send Data." << std::endl;

    // 投递到 io_context
    boost::asio::post(_ioc, [this, self, node]()
                      { DoSend(node); });
}

void CSession::Send(const MessageHeader &header, const std::string &body)
{
    Send(header, body.c_str(), body.length());
}

void CSession::Send(const MessageHeader &header, const nlohmann::json &body)
{
    // 发送
    Send(header, body.dump(4));
}

void CSession::ClientClose()
{
    LOG_INFO << "CoroutineSession: Client Close a Connect." << std::endl;
    Close();
    this->_server->DelSessionByUuid(this->_uuid);
}

bool CSession::SendToOtherSession(const std::string &uuid, const MessageHeader &header, const std::string &body)
{
    // 获取其他会话
    auto session = this->_server->GetSessionByUuid(uuid);

    // 如果不存在，则返回 false
    if (session == nullptr)
        return false;

    LOG_INFO << "SendToOtherSession: " << uuid << std::endl;
    LOG_INFO << "SendToOtherSession: " << session->GetSocket().remote_endpoint() << std::endl;
    // 发送信息
    session->Send(header, body);

    return true;
}

void CSession::HandleWrite(const boost::system::error_code &error)
{
    try
    {
        if (!error)
        {
            // 加锁 (保护队列操作)
            std::unique_lock<std::mutex> lock(this->_mutex);

            // 1. 移除刚刚发送完成的节点
            if (!this->_sendQue.empty())
            {
                this->_sendQue.pop();
            }

            // 2. 检查是否还有待发送的消息
            if (!this->_sendQue.empty())
            {
                // 获取下一个节点
                auto msgNode = this->_sendQue.front();

                // 解锁 (async_write 不需要持有锁，且 msgNode 是 shared_ptr 安全的)
                lock.unlock();

                // 3. 必须发送 Header + Body (GetSendData)，不能只发 Body
                boost::asio::async_write(
                    this->_socket,
                    boost::asio::buffer(msgNode->GetSendData(), msgNode->GetSendSize()),
                    std::bind(&CSession::HandleWrite, shared_from_this(), std::placeholders::_1));
            }
        }
        else
        {
            // 忽略连接被中止的错误(1236/995等)，这通常意味着连接已关闭
            if (error.value() != boost::asio::error::operation_aborted)
            {
                LOG_ERROR << "HandleWrite Get Error : " << error.message() << " [" << error.value() << "]" << std::endl;
            }
            // 出错时清理会话
            Close();
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "HandleWrite Exception: " << e.what() << '\n';
    }
}

void CSession::DoSend(std::shared_ptr<MsgNode> node)
{
    if (this->_bStop)
        return;

    // 加锁 (保护队列操作)
    std::unique_lock<std::mutex> lock(this->_mutex);

    // 检查队列长度限制
    if (this->_sendQue.size() >= MAX_SENDQUE_LEN)
    {
        LOG_WARN << "Session " << this->_uuid << " send queue full\n";
        return;
    }

    // ✅ 修复：在 push 之前判断是否需要触发 Write
    bool writing = !this->_sendQue.empty();
    this->_sendQue.push(node);

    // 如果之前队列为空，说明当前没有 Write 任务在运行，需要主动触发
    if (!writing)
    {
        // 释放锁后再调用 DoWrite，防止 DoWrite 内部死锁 (虽然目前 DoWrite 逻辑简单，但这是好习惯)
        lock.unlock();
        DoWrite();
    }
}

void CSession::DoWrite()
{
    // 加锁 (保护队列操作)
    std::unique_lock<std::mutex> lock(this->_mutex);

    if (this->_sendQue.empty())
        return;

    auto self = shared_from_this();
    auto node = _sendQue.front(); // 获取 shared_ptr 副本，确保在 async_write 期间存活

    // 解锁
    lock.unlock();

    // 发送
    boost::asio::async_write(
        this->_socket,
        boost::asio::buffer(node->GetSendData(), node->GetSendSize()), // 需发送：Header + Body
        [this, self](const boost::system::error_code &ec, std::size_t)
        {
            HandleWrite(ec);
        });
}