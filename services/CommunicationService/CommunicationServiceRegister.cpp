
#ifndef COMMUNICATIONSERVICEREGISTER_CPP
#define COMMUNICATIONSERVICEREGISTER_CPP

#include "CommunicationService.h"
#include "../ServiceManager.h"

namespace
{
    struct CommunicationServiceRegister
    {
        CommunicationServiceRegister()
        {
            ServiceManager::GetInstance().RegisterService(
                std::make_shared<CommunicationService>());
        }
    };

    // 全局静态对象，程序启动时自动执行
    CommunicationServiceRegister _communication_service_register;
}

#endif // COMMUNICATIONSERVICEREGISTER_CPP