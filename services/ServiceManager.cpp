#include "ServiceManager.h"

#include "../infra/log/Logger.h"

#include <iostream>

ServiceManager::~ServiceManager()
{
    ClearService();
}

ServiceManager &ServiceManager::GetInstance()
{
    static ServiceManager instance;
    return instance;
}

void ServiceManager::RegisterService(std::shared_ptr<IService> service)
{
    // 安全性检查
    if (service == nullptr)
        return;

    // 加锁
    std::lock_guard<std::mutex> lock(this->_mutex);
    // 获取 serviceId
    auto serviceId = service->GetServiceId();
    // 检查服务是否已经存在
    if (this->_serviceMap.count(serviceId) > 0)
    {
        LOG_WARN << "Service " << serviceId << " already registered " << std::endl;
        return;
    }
    // 对服务的命令进行注册
    service->RegisterCmd();
    // 注册服务
    this->_serviceMap[serviceId] = service;
}

std::shared_ptr<IService> ServiceManager::GetServiceById(const uint16_t serviceId)
{
    // 加锁
    std::lock_guard<std::mutex> lock(this->_mutex);
    
    auto iter = this->_serviceMap.find(serviceId);
    if (iter != this->_serviceMap.end())
    {
        return iter->second;
    }

    LOG_WARN << "Service " << serviceId << " Not found " << std::endl;
    return nullptr;
}

void ServiceManager::ClearService()
{
    this->_serviceMap.clear();
}
