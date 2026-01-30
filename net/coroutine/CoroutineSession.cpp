
#include "CoroutineSession.h"

#include "../../core/server/CServer.h"
#include "../../core/logic/LogicNode.h"
#include "../../core/logic/LogicSystem.h"

#include "../../infra/log/Logger.h"
#include "../../infra/util/StringFormat.h"

#include "../../core/common/Const.h" // 获取心跳服务ID

CoroutineSession::CoroutineSession(boost::asio::io_context &ioc, CServer *server)
    : CSession(ioc, server),
      _deadline(ioc) // 初始化定时器
{
    // 初始化最后活动时间
    _last_message_time = std::chrono::steady_clock::now();
}

CoroutineSession::~CoroutineSession()
{
    // 取消定时器
    _deadline.cancel();
}

// 新增：心跳检测协程
void CoroutineSession::StartHeartbeatCheck()
{
    // 设置超时时间，例如 60 秒无数据则断开
    auto self = shared_from_this();
    boost::asio::co_spawn(_ioc, [self, this]() -> boost::asio::awaitable<void>
                          {
        while (!_bStop) {
            // 设置定时器等待 5 秒检查一次
            _deadline.expires_after(std::chrono::seconds(5));
            boost::system::error_code ec;
            co_await _deadline.async_wait(redirect_error(boost::asio::use_awaitable, ec));

            if (_bStop) co_return;

            // 检查距离上次收到消息是否超过 60 秒
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - _last_message_time).count();

            if (duration > 60) {
                LOG_WARN << "Client heartbeat timeout! Force Close. UUID: " << GetUuid() << std::endl;
                Close();
                _server->DelSessionByUuid(_uuid);
                co_return;
            }
        } }, boost::asio::detached);
}

void CoroutineSession::Start()
{
    // 1. 启动看门狗
    StartHeartbeatCheck();

    // 开启协程
    boost::asio::co_spawn(this->_ioc, [this]() -> boost::asio::awaitable<void>
                          {
                 try
                 {
                    for (; !this->_bStop; )
                    {
                        // 重置消息节点
                        this->_recvNode->Clear();
                        // 开始读取头部信息
                        std::size_t len = co_await boost::asio::async_read(this->_socket, boost::asio::buffer(this->_recvNode->GetHeaderData(), MsgNode::GetHeaderSize()), boost::asio::use_awaitable);
                        if (len == 0)
                        {
                            LOG_ERROR << "CoroutineSession: Client Close a Connect." << std::endl;
                            Close();
                            this->_server->DelSessionByUuid(this->_uuid);
                            co_return;
                        }

                        // 转为本地主机字节序 必须转换！！！
                        this->_recvNode->GetHeader().ToHost();

                        // 为消息节点分配内存
                        this->_recvNode->Allocate(this->_recvNode->GetHeader().length);

                        // 开始读取消息信息
                        len = co_await boost::asio::async_read(this->_socket, boost::asio::buffer(this->_recvNode->GetBody(), this->_recvNode->GetBodyLen()), boost::asio::use_awaitable);
                        if (len == 0)
                        {
                            LOG_ERROR << "CoroutineSession: Client Close a Connect." << std::endl;
                            Close();
                            this->_server->DelSessionByUuid(this->_uuid);
                            co_return;
                        }

                        LOG_INFO << "CoroutineSession: Recived Node to LogicSystem... " << std::endl;
                        // 输出信息
                        this->_recvNode->Print();
                        auto msg = std::move(_recvNode);
                        this->_recvNode = std::make_shared<MsgNode>();

                        // TODO: 将消息节点传递给逻辑处理系统
                        LogicSystem::GetInstance().PostMsgToQue(std::make_shared<LogicNode>(shared_from_this(), msg));

                    }
                 }
                 catch (boost::system::system_error& e)
                 {
                    if (e.code().value() == 10054) {
                        LOG_INFO << "客户端正常断开连接" << '\n';
                    } 
                    else 
                    {
                        LOG_ERROR << "网络错误 [" << e.code().value() << "]: " 
                                << ConvertStringToUTF8(e.code().message()) << '\n';
                    }
                     Close();
                     this->_server->DelSessionByUuid(this->_uuid);
                 } }, boost::asio::detached);
}
