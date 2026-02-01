
#include "CoroutineSession.h"

#include "../../core/server/CServer.h"

#include "../../services/ServiceManager.h"
#include "../../services/IService.h"

#include "../../infra/log/Logger.h"
#include "../../infra/util/StringFormat.h"

#include "../../core/common/Const.h" // 获取心跳服务ID

CoroutineSession::CoroutineSession(boost::asio::io_context &ioc, boost::asio::ip::tcp::socket socket, CServer *server)
    : CSession(ioc, std::move(socket), server),
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

                        // 更新最后活动时间（喂狗）
                        this->_last_message_time = std::chrono::steady_clock::now();
                        if (len == 0)
                        {
                            LOG_ERROR << "CoroutineSession: Client Close a Connect." << std::endl;
                            Close();
                            this->_server->DelSessionByUuid(this->_uuid);
                            co_return;
                        }

                        // 转为本地主机字节序 必须转换！！！
                        this->_recvNode->GetHeader().ToHost();

                        // 获取消息长度
                        auto length = this->_recvNode->GetHeader().length;

                        // 为消息节点分配内存
                        this->_recvNode->Allocate(length);
                        
                        // 如果消息长度大于 0，则开始读取消息体
                        if (length > 0)
                        {
                            // 开始读取消息信息
                            len = co_await boost::asio::async_read(this->_socket, boost::asio::buffer(this->_recvNode->GetBody(), this->_recvNode->GetBodyLen()), boost::asio::use_awaitable);
                            // 如果本来想读数据却读到了 0，那才是真正的断开
                            if (len == 0)
                            {
                                LOG_ERROR << "CoroutineSession: Client Close a Connect." << std::endl;
                                Close();
                                this->_server->DelSessionByUuid(this->_uuid);
                                co_return;
                            }
                        }

                        // 输出信息
                        this->_recvNode->Print();
                        // 获取头部信息，直接分发，避免 LogicSystem 阻塞
                        auto& header = this->_recvNode->GetHeader();

                        // 1. 查找服务
                        auto service = ServiceManager::GetInstance().GetServiceById(header.serviceId);
                        // 如果服务存在
                        if (service) 
                        {
                            // 2. 构造消息，使用移动语义，避免拷贝
                            auto msg = std::move(this->_recvNode);
                            // 重新构建消息节点
                            this->_recvNode = std::make_shared<MsgNode>();
                            
                            // 3. 执行业务逻辑
                            // 选项 A (推荐): 并发处理
                            // 使用 co_spawn 启动一个新的协程来处理业务。
                            // 优点：当前读循环可以立即继续，处理下一个包（高吞吐）。
                            // 缺点：同一个连接的请求可能会乱序完成（如果业务耗时不同）。
                            // 注意：这里需要传入 shared_from_this() 保持 Session 存活
                            boost::asio::co_spawn(this->_ioc, 
                                service->Handle(shared_from_this(), msg), 
                                boost::asio::detached);

                            /* // 选项 B: 顺序处理
                            // 使用 co_await 等待业务处理完成。
                            // 优点：严格保证同一个连接的请求按顺序处理。
                            // 缺点：如果某个请求处理慢，会阻塞后续包的读取。
                            
                            co_await service->Handle(shared_from_this(), msg);
                            */
                        }
                        else
                        {
                            LOG_WARN << "Service Not Found: " << header.serviceId << std::endl;
                        }
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
