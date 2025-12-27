

#ifndef SERVICES_ISERVICE_H
#define SERVICES_ISERVICE_H

#include <memory>
#include <cstdint>
#include <functional>
#include <unordered_map>

#include <boost/asio.hpp>

#include "../core/protocol/MessageHeader.h"
#include "../core/message/MsgNode.h"
#include "../core/common/Const.h"
#include "../core/session/CSession.h"

class IService
{
public:
    // 回调函数模版
    typedef std::function<boost::asio::awaitable<void>(std::shared_ptr<CSession>, std::shared_ptr<MsgNode> msg)> CmdFunCallBack;

    virtual ~IService() = default;

    // 纯虚函数 获取 serviceId
    virtual uint16_t GetServiceId() const = 0;

    // 纯虚函数 注册 cmd
    virtual void RegisterCmd() = 0;

    // 子类公用方法 分发 cmd
    boost::asio::awaitable<void> Handle(std::shared_ptr<CSession> session, std::shared_ptr<MsgNode> msg);

protected:
    // 命令回调字典 子类使用
    std::unordered_map<uint16_t, CmdFunCallBack> _cmdMap;
};

#endif // SERVICES_ISERVICE_H