
#ifndef COMMUNICATIONSERVICE_H
#define COMMUNICATIONSERVICE_H

#include "../IService.h"

class CommunicationService : public IService
{
public:
    CommunicationService();

    uint16_t GetServiceId() const override;

    void RegisterCmd() override;

private:
    // 创建连接回调方法
    boost::asio::awaitable<void> OnCreateCallBack(std::shared_ptr<CSession>, std::shared_ptr<MsgNode>);
    // 关闭连接回调方法
    boost::asio::awaitable<void> OnCloseCallBack(std::shared_ptr<CSession>, std::shared_ptr<MsgNode>);
    // 发送消息回调方法
    boost::asio::awaitable<void> OnSendCallBack(std::shared_ptr<CSession>, std::shared_ptr<MsgNode>);
    // 显示信息回调方法
    boost::asio::awaitable<void> OnShowCallBack(std::shared_ptr<CSession>, std::shared_ptr<MsgNode>);
};

#endif // COMMUNICATIONSERVICE_H
