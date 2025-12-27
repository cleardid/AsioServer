
#ifndef DBSERVICE_REGISTER_CPP
#define DBSERVICE_REGISTER_CPP

#include "DBService.h"
#include "../ServiceManager.h"

namespace
{
    struct DBServiceRegister
    {
        DBServiceRegister()
        {
            ServiceManager::GetInstance().RegisterService(
                std::make_shared<DBService>());
        }
    };

    // 全局静态对象，程序启动时自动执行
    DBServiceRegister _db_service_register;
}

#endif // DBSERVICE_REGISTER_CPP