
#include "DBConnectionPool.h"
#include "DBConnection.h"

#include "../../infra/log/Logger.h"

DBConnectionPool::DBConnectionPool(const std::size_t maxConn)
    : _max(maxConn),
      _closed(false),
      _created(0)
{
}

std::shared_ptr<DBConnection> DBConnectionPool::Acquire(std::chrono::milliseconds timeout)
{
    // 加锁
    std::unique_lock<std::mutex> lock(this->_mutex);

    // 记录等待起点
    auto deadline = std::chrono::steady_clock::now() + timeout;

    // 如果连接池已关闭，返回空
    if (this->_closed)
    {
        LOG_WARN << "DBConnectionPool is closed, cannot acquire connection." << std::endl;
        return nullptr;
    }

    while (true)
    {
        // 1. 如果有空闲连接，直接返回
        if (!_idle.empty())
        {
            auto conn = _idle.front();
            _idle.pop();
            return conn;
        }

        // 2. 如果没有空闲连接，但未达到最大连接数，创建新连接
        if (this->_created < this->_max)
        {
            this->_created++;
            // 解锁后再创建（避免阻塞其他线程）
            lock.unlock();

            // 创建新连接
            auto conn = this->CreateConnection();
            // 安全性检查
            if (!conn)
            {
                // 创建失败，回滚
                lock.lock();
                this->_created--;
                LOG_ERROR << "Failed to create new DBConnection." << std::endl;
                this->_cond.notify_one();
                return nullptr;
            }

            return conn;
        }

        // 3. 等待连接可用或超时
        if (this->_cond.wait_until(lock, deadline) == std::cv_status::timeout)
        {
            // 超时，返回空
            LOG_WARN << "Timeout while waiting for DBConnection." << std::endl;
            return nullptr;
        }
    }
}

void DBConnectionPool::Release(std::shared_ptr<DBConnection> conn)
{
    // 加锁
    std::lock_guard<std::mutex> lock(this->_mutex);

    // 如果连接池已关闭，返回空
    if (this->_closed)
    {
        LOG_WARN << "DBConnectionPool is closed, cannot release connection." << std::endl;
        return;
    }

    // 将连接放回空闲连接中
    this->_idle.push(conn);
    // 通知等待的线程有连接可用
    this->_cond.notify_one();
}

void DBConnectionPool::CloseAll()
{
    // 加锁
    std::lock_guard<std::mutex> lock(this->_mutex);

    this->_closed = true;

    // 清空所有连接
    while (!this->_idle.empty())
        this->_idle.pop();

    this->_cond.notify_all();
}