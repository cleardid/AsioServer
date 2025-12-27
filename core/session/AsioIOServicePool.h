
#ifndef ASIOIOSERVICEPOOL_H
#define ASIOIOSERVICEPOOL_H

// 使用 c++20 特性创建单例类

#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <boost/asio.hpp>

class AsioIOServicePool
{
public:
    using IOService = boost::asio::io_context;
    using Work = boost::asio::io_context::work;
    using WorkPtr = std::unique_ptr<Work>;

    ~AsioIOServicePool();
    // 删除拷贝构造函数
    AsioIOServicePool(const AsioIOServicePool &) = delete;
    // 删除赋值构造函数
    AsioIOServicePool &operator=(const AsioIOServicePool &) = delete;

    // 对外接口
    // 获取 IOService
    boost::asio::io_context &GetIOServive();
    // 停止
    void Stop();

    // 静态方法，获取单例(使用 c++20 特性)
    static AsioIOServicePool &GetInstance();

private:
    // 私有构造函数
    AsioIOServicePool(std::size_t size);

    // 服务数组
    std::vector<IOService> _ioServices;
    std::vector<WorkPtr> _works;
    std::vector<std::thread> _threads;
    std::size_t _nextIndex;
    std::size_t _maxSize;
};

#endif // ASIOIOSERVICEPOOL_H