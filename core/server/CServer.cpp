
#include "CServer.h"

#include "../session/CSession.h"
#include "../session/AsioIOServicePool.h"

#include "../../net/coroutine/CoroutineSession.h"
#include "../../net/threaded/AsyncSession.h"

#include "../../infra/log/Logger.h"

CServer::CServer(boost::asio::io_context &ioc, const uint16_t port, ASIO_TYPE type)
    : _port(port),
      _ioContext(ioc),
      _acceptor(ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
      _type(type),
      _isFirstCreate(true)
{
    LOG_INFO << "Server Start in " << port << std::endl;
    // 启动连接
    switch (this->_type)
    {
    case ASIO_TYPE::ASYNC:
        StartAsyncAccpet();
        break;
    case ASIO_TYPE::COROUTINE:
        StartCoroutineAccpet();
        break;
    // 默认使用协程模式
    default:
        StartCoroutineAccpet();
        break;
    }

    this->_isFirstCreate = false;
}

CServer::~CServer()
{
    LOG_INFO << "Server is destructed." << std::endl;
    ClearSession();
}

void CServer::ClearSession()
{
    this->_sessionMap.clear();
}

void CServer::DelSessionByUuid(const std::string &uuid)
{
    std::lock_guard<std::mutex> lock(this->_mutex);
    this->_sessionMap.erase(uuid);
}

std::shared_ptr<CSession> CServer::GetSessionByUuid(const std::string &uuid)
{
    // 加锁
    std::lock_guard<std::mutex> lock(this->_mutex);
    // 查找
    auto it = this->_sessionMap.find(uuid);
    if (it != this->_sessionMap.end())
    {
        return it->second;
    }
    return nullptr;
}

void CServer::StartCoroutineAccpet()
{
    if (this->_isFirstCreate)
        LOG_INFO << "StartCoroutineAccpet " << std::endl;
    // 从会话池中获取一个新会话
    auto &ioc = AsioIOServicePool::GetInstance().GetIOServive();
    // 获取指针，用于 lambda 安全捕获
    boost::asio::io_context *pIoc = &ioc;

    // 创建轻量级 socket
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(ioc);
    // 开始异步监听
    this->_acceptor.async_accept(*socket,
                                 [this, socket, pIoc](const boost::system::error_code &error)
                                 {
                                     if (!error)
                                     {
                                         // 2. 构造 Session，使用 *pIoc 安全地访问上下文
                                         // 注意：这里需要解引用 socket 指针并 move
                                         std::shared_ptr<CSession> newSession = std::make_shared<CoroutineSession>(*pIoc, std::move(*socket), this);

                                         newSession->Start();
                                         // 输出日志信息
                                         LOG_INFO << "Get Connention From " << newSession->GetSocket().remote_endpoint() << std::endl;
                                         {
                                             std::lock_guard<std::mutex> lock(this->_mutex);
                                             this->_sessionMap.insert(std::make_pair(newSession->GetUuid(), newSession));
                                         }
                                     }
                                     else
                                     {
                                         LOG_WARN << "Accept Error: " << error.message() << std::endl;
                                         // 特殊处理文件描述符耗尽的情况
                                         if (error == boost::asio::error::no_descriptors)
                                         {
                                             // 休眠 100ms 再重试，避免 CPU 空转
                                             std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                         }
                                     }

                                     StartCoroutineAccpet();
                                 });
}

void CServer::StartAsyncAccpet()
{
    if (this->_isFirstCreate)
        LOG_INFO << "StartAsyncAccpet " << std::endl;
    // 从会话池中获取一个新会话
    auto &ioc = AsioIOServicePool::GetInstance().GetIOServive();
    // 获取指针，用于 lambda 安全捕获
    boost::asio::io_context *pIoc = &ioc;

    // 创建轻量级 socket
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(ioc);
    // 开始异步监听
    this->_acceptor.async_accept(*socket,
                                 [this, socket, pIoc](const boost::system::error_code &error)
                                 {
                                     if (!error)
                                     {
                                         // 2. 构造 Session，使用 *pIoc 安全地访问上下文
                                         // 注意：这里需要解引用 socket 指针并 move
                                         std::shared_ptr<CSession> newSession = std::make_shared<AsyncSession>(*pIoc, std::move(*socket), this);

                                         newSession->Start();
                                         // 输出日志信息
                                         LOG_INFO << "Get Connention From " << newSession->GetSocket().remote_endpoint() << std::endl;
                                         {
                                             std::lock_guard<std::mutex> lock(this->_mutex);
                                             this->_sessionMap.insert(std::make_pair(newSession->GetUuid(), newSession));
                                         }
                                     }
                                     else
                                     {
                                         LOG_WARN << "Accept Error: " << error.message() << std::endl;
                                         // 特殊处理文件描述符耗尽的情况
                                         if (error == boost::asio::error::no_descriptors)
                                         {
                                             // 休眠 100ms 再重试，避免 CPU 空转
                                             std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                         }
                                     }

                                     StartAsyncAccpet();
                                 });
}