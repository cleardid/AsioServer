
#include "LogicSystem.h"
#include "LogicNode.h"

#include "../common/Const.h"
#include "../session/CSession.h"

#include "../../infra/log/Logger.h"

#include "../../services/IService.h"
#include "../../services/ServiceManager.h"

#include <boost/asio/co_spawn.hpp>

LogicSystem::LogicSystem()
    : _isStoped(false)
{
    // 启动工作线程
    this->_wordThread = std::thread(&LogicSystem::ProcessMsg, this);
}

// 析构函数
LogicSystem::~LogicSystem()
{
    // 更改运行状态
    this->_isStoped = true;
    //
    this->_workCond.notify_one();
    // 等待线程结束
    this->_wordThread.join();
}

// 单例
LogicSystem &LogicSystem::GetInstance()
{
    static LogicSystem instance = LogicSystem();
    return instance;
}

// 对外接口
void LogicSystem::PostMsgToQue(std::shared_ptr<LogicNode> msgNode)
{
    // 加锁
    std::unique_lock<std::mutex> lock(this->_queMutex);
    this->_msgQue.push(msgNode);

    // 如果消息队列恰好为 1，则唤醒工作线程
    if (this->_msgQue.size() == 1)
    {
        this->_workCond.notify_one();
    }
}

// 处理消息
void LogicSystem::ProcessMsg()
{
    for (;;)
    {
        // 加锁
        std::unique_lock<std::mutex> lock(this->_queMutex);

        // 如果队列为空 并且 未停止，则挂起线程，避免反复轮转
        while (this->_msgQue.empty() && !this->_isStoped)
        {
            this->_workCond.wait(lock);
        }

        // 如果停止
        if (this->_isStoped)
        {
            // 队列非空
            while (!this->_msgQue.empty())
            {
                if (!AssistProcessMsg())
                    continue;
            }
            // 退出最外围的 for 循环
            break;
        } // if (this->_isStoped)

        if (!AssistProcessMsg())
            continue;
    }
}

bool LogicSystem::AssistProcessMsg()
{
    // 此时队列不为空
    // 获取头节点
    auto node = _msgQue.front();
    // 弹出头节点
    _msgQue.pop();

    auto session = node->GetSession();
    auto msgNode = node->GetRecvNode();
    auto &header = msgNode->GetHeader();

    // 根据 serviceId 找 Service
    auto service = ServiceManager::GetInstance().GetServiceById(header.serviceId);
    // 不存在服务
    if (!service)
    {
        LOG_WARN << "No service for serviceId = "
                 << header.serviceId << std::endl;
        return false;
    }

    //
    LOG_INFO << "LogicSystem: Dispatch to ServiceId = "
             << header.serviceId << ", CmdId = "
             << header.cmdId << std::endl;

    // 获取socket
    auto &socket = session->GetSocket();
    LOG_INFO << "Get Connention By " << socket.remote_endpoint() << std::endl;

    // ==============================
    // 投递到 asio 协程执行
    // ==============================
    // 注意：
    // - LogicSystem 线程 ≠ asio IO 线程
    // - Service::Handle 是 awaitable，不能直接调用
    // - 必须在 session 所属 io_context 上 co_spawn
    //

    boost::asio::co_spawn(
        session->GetIoContext(), // 绑定到该连接的 io_context
        service->Handle(session, msgNode),
        boost::asio::detached // 不需要 join / future
    );

    return true;
}