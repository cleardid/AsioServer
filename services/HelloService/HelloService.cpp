#include "HelloService.h"
#include <iostream>

#include "../../infra/log/Logger.h"

HelloService::HelloService()
{
    // // 注册消息
    // RegisterCmd();
}

uint16_t HelloService::GetServiceId() const
{
    return SERVICE_HELLO;
}

void HelloService::RegisterCmd()
{
    this->_cmdMap[HELLO_CMD_TEST] = std::bind(&HelloService::OnHelloCallBack, this,
                                              std::placeholders::_1, std::placeholders::_2);
}

boost::asio::awaitable<void> HelloService::OnHelloCallBack(std::shared_ptr<CSession> session, std::shared_ptr<MsgNode> msg)
{
    LOG_INFO << "Call OnHelloCallBack " << std::endl;

    // 获取消息体
    std::string text(msg->GetBody(), msg->GetBodyLen());

    // 回包
    MessageHeader rsp{};
    rsp.magic = 0x55AA;
    rsp.version = 1;
    rsp.serviceId = GetServiceId();
    rsp.cmdId = HELLO_CMD_TEST;
    rsp.length = text.size();
    rsp.seq = msg->GetHeader().seq;

    // 执行回调
    session->Send(rsp, text);
    co_return;
}
