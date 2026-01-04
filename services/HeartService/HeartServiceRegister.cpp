
#ifndef HEART_SERVICE_REGISTER_CPP
#define HEART_SERVICE_REGISTER_CPP

#include "HeartService.h"
#include "../ServiceManager.h"

namespace
{
    struct HeartBeatServiceRegister
    {
        HeartBeatServiceRegister()
        {
            ServiceManager::GetInstance().RegisterService(
                std::make_shared<HeartService>());
        }
    };

    // 全局静态对象，程序启动时自动执行
    HeartBeatServiceRegister _heartbeat_service_register;
}

#endif // HEART_SERVICE_REGISTER_CPP