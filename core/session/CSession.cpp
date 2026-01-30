
#include "CSession.h"
#include "../common/Const.h"
#include "../message/MsgNode.h"
#include "../server/CServer.h"

#include "../../infra/log/Logger.h"
#include "../../services/CommunicationService/ClientInfo.h"
#include "../../services/CommunicationService/ClientManager.h"

CSession::CSession(boost::asio::io_context &ioc, CServer *server)
    : _ioc(ioc),
      _server(server),
      _socket(ioc),
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

        // 自动清理在线列表
        if (_clientInfo) // 假设你有这个成员存储 ClientInfo
        {
            LOG_INFO << "Auto removing client: " << _clientInfo->GetName() << std::endl;
            ClientManager::GetInstance().RemoveClient(_clientInfo->GetName());
            // 清空指针
            _clientInfo.reset();
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << "Destructor error: " << e.what() << '\n';
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
            // 加锁
            std::unique_lock<std::mutex> lock(this->_mutex);
            // 上一个头部已经发送完毕，则出队即可
            this->_sendQue.pop();
            if (!this->_sendQue.empty())
            {
                // 获取当前节点
                auto msgNode = this->_sendQue.front();
                // 解锁
                lock.unlock();
                // 发送信息
                boost::asio::async_write(this->_socket, boost::asio::buffer(msgNode->GetBody(), msgNode->GetBodyLen()), std::bind(&CSession::HandleWrite, shared_from_this(), std::placeholders::_1));
            }
        }
        else
        {
            LOG_ERROR << "HandleWrite Get Error : " << error.what() << std::endl;
        }
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << e.what() << '\n';
    }
}

void CSession::DoSend(std::shared_ptr<MsgNode> node)
{
    if (this->_bStop)
        return;

    if (this->_sendQue.size() >= MAX_SENDQUE_LEN)
    {
        LOG_WARN << "Session " << this->_uuid << " send queue full\n";
        return;
    }

    bool writing = !this->_sendQue.empty();
    this->_sendQue.push(node);

    if (!writing)
    {
        DoWrite();
    }
}

void CSession::DoWrite()
{
    if (this->_sendQue.empty())
        return;

    auto self = shared_from_this();
    auto &node = _sendQue.front();

    boost::asio::async_write(
        this->_socket,
        boost::asio::buffer(node->GetSendData(), node->GetSendSize()),
        [this, self](const boost::system::error_code &ec, std::size_t)
        {
            HandleWrite(ec);
        });
}
