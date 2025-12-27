
#ifndef SERVICE_MANAGER_H
#define SERVICE_MANAGER_H

#include "IService.h"

#include <mutex>

class ServiceManager
{
private:
    /* data */
public:
    // 删除拷贝构造函数
    ServiceManager(const ServiceManager &) = delete;
    // 删除赋值构造函数
    ServiceManager &operator=(const ServiceManager &) = delete;

    // 析构函数
    ~ServiceManager();

    // 静态方法 获取单例
    static ServiceManager &GetInstance();

    // 注册服务
    void RegisterService(std::shared_ptr<IService> service);
    // 获取服务
    std::shared_ptr<IService> GetServiceById(const uint16_t serviceId);
    // 清空服务
    void ClearService();

private:
    ServiceManager() = default;
    std::unordered_map<uint16_t, std::shared_ptr<IService>> _serviceMap;
    // 线程安全锁
    std::mutex _mutex;
};

#endif // SERVICE_MANAGER_H
