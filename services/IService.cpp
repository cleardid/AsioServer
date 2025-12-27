
#include "IService.h"

#include "../infra/log/Logger.h"

// 分发 cmd
boost::asio::awaitable<void> IService::Handle(std::shared_ptr<CSession> session, std::shared_ptr<MsgNode> msg)
{
    // 获取头部信息
    auto &header = msg->GetHeader();

    // 获取命令 ID
    auto cmdId = header.cmdId;

    // 尝试获取命令回调
    auto it = this->_cmdMap.find(cmdId);
    if (it != this->_cmdMap.end())
    {
        // 执行回调
        co_await it->second(session, msg);
    }
    // 没有找到回调
    else
    {
        LOG_WARN << "HelloService: Not found cmdId " << cmdId << std::endl;
    }

    co_return;
}