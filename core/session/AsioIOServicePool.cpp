
#include "AsioIOServicePool.h"

#include "../../infra/log/Logger.h"

#include "../../config/ConfigReader.h"

#include <filesystem>
#include <fstream>

// 私有构造函数
AsioIOServicePool::AsioIOServicePool(std::size_t size)
    : _maxSize(size),
      _nextIndex(0),
      _ioServices(size),
      _works(size)
{
    LOG_INFO << "IOService Count is " << size << std::endl;
    // 赋值work，使得每个 work 管理各自的 iocontext
    for (size_t i = 0; i < size; i++)
    {
        this->_works[i] = std::unique_ptr<Work>(new Work(this->_ioServices[i]));
    }

    // 开启各自的线程
    for (size_t i = 0; i < size; i++)
    {
        this->_threads.emplace_back([this, i]()
                                    { this->_ioServices[i].run(); });
    }
}

AsioIOServicePool::~AsioIOServicePool()
{
    LOG_INFO << "~AsioIOServicePool is destructed." << std::endl;
}

boost::asio::io_context &AsioIOServicePool::GetIOServive()
{
    return this->_ioServices[this->_nextIndex++ % this->_maxSize];
}

void AsioIOServicePool::Stop()
{
    for (auto &work : this->_works)
    {
        // 释放 unique_ptr，调用析构，从而关闭 iocontext
        work.reset();
    }

    // 等待线程退出
    for (auto &t : this->_threads)
    {
        t.join();
    }
}

AsioIOServicePool &AsioIOServicePool::GetInstance()
{
    // 获取硬件最大并发数
    unsigned int maxSize = std::thread::hardware_concurrency();

    try
    {
        // 创建配置文件读取器
        auto configReader = std::make_shared<ConfigReader>("../config/server.json");
        // 首先尝试获取配置文件中的线程池大小
        auto setSize = configReader->GetUInt("thread_pool_size").value_or(maxSize / 2);

        if (setSize > 0 && setSize <= maxSize)
        {
            maxSize = setSize;
        }
        else
        {
            maxSize /= 2;
            LOG_WARN << "Invalid thread pool size in config file, use default value: " << maxSize << std::endl;
        }

        // 释放配置文件读取器
        configReader.reset();
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << e.what() << '\n';
    }

    // 启动所有线程池
    static AsioIOServicePool instance = AsioIOServicePool(maxSize);
    return instance;
}