
#ifndef HELLO_SERVICE_H
#define HELLO_SERVICE_H

#include "../IService.h"

class HelloService : public IService
{
public:
    HelloService();

    uint16_t GetServiceId() const override;

    void RegisterCmd() override;

private:
    boost::asio::awaitable<void> OnHelloCallBack(std::shared_ptr<CSession> session, std::shared_ptr<MsgNode> msg);
};

#endif // HELLO_SERVICE_H