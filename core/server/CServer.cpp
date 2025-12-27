
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
      _type(type)
{
    LOG_INFO << "Server Start in " << port << std::endl;
    // 启动连接
    AssistStart(true);
}

CServer::~CServer()
{
    LOG_INFO << "Server is destructed." << std::endl;
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

void CServer::StartCoroutineAccpet(bool isCreateFunc)
{
    if (isCreateFunc)
        LOG_INFO << "StartCoroutineAccpet " << std::endl;
    // 从会话池中获取一个新会话
    auto &ioc = AsioIOServicePool::GetInstance().GetIOServive();
    // 创建一个新会话
    std::shared_ptr<CSession> newSession = std::make_shared<CoroutineSession>(ioc, this);
    // 开始异步监听
    this->_acceptor.async_accept(newSession->GetSocket(), std::bind(&CServer::HandleAccpet, this, newSession, std::placeholders::_1));
}

void CServer::StartAsyncAccpet(bool isCreateFunc)
{
    if (isCreateFunc)
        LOG_INFO << "StartAsyncAccpet " << std::endl;
    // 从会话池中获取一个新会话
    auto &ioc = AsioIOServicePool::GetInstance().GetIOServive();
    // 创建一个新会话
    std::shared_ptr<CSession> newSession = std::make_shared<AsyncSession>(ioc, this);
    // 开始异步监听
    this->_acceptor.async_accept(newSession->GetSocket(), std::bind(&CServer::HandleAccpet, this, newSession, std::placeholders::_1));
}

void CServer::HandleAccpet(std::shared_ptr<CSession> newSession, const boost::system::error_code &error)
{
    if (!error)
    {
        // 启动会话
        newSession->Start();
        // 加锁
        std::lock_guard<std::mutex> lock(this->_mutex);
        // 加入字典
        this->_sessionMap.insert(std::make_pair(newSession->GetUuid(), newSession));
    }
    else
    {
        LOG_WARN << "HandleAccpet Get Error " << error.message() << std::endl;
    }

    // 继续接收连接
    AssistStart();
}

void CServer::AssistStart(bool isCreateFunc)
{
    switch (this->_type)
    {
    case ASIO_TYPE::ASYNC:
        StartAsyncAccpet(isCreateFunc);
        break;
    case ASIO_TYPE::COROUTINE:
        StartCoroutineAccpet(isCreateFunc);
        break;
    // 默认使用协程模式
    default:
        StartCoroutineAccpet(isCreateFunc);
        break;
    }
}
