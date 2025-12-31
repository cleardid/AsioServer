
#ifndef HEARTSERVICE_H
#define HEARTSERVICE_H

#include "../IService.h"

class HeartService : public IService
{
public:
    HeartService();

    uint16_t GetServiceId() const override;

    void RegisterCmd() override;

private:
    boost::asio::awaitable<void> OnRecvHeartCallBack(std::shared_ptr<CSession>, std::shared_ptr<MsgNode>);
};

#endif // HEARTSERVICE_H