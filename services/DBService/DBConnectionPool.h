
#ifndef DBCONNECTIONPOOL_H
#define DBCONNECTIONPOOL_H

#include <queue>
#include <mutex>
#include <memory>

// 前置声明
class DBConnection;

class DBConnectionPool
{
public:
    // 删除默认构造函数
    DBConnectionPool() = delete;
    // 删除拷贝构造函数
    DBConnectionPool(const DBConnectionPool &) = delete;
    // 删除移动构造函数
    DBConnectionPool(DBConnectionPool &&) = delete;
    // 删除赋值运算符重载函数
    DBConnectionPool &operator=(const DBConnectionPool &) = delete;
    // 删除移动赋值运算符重载函数
    DBConnectionPool &operator=(DBConnectionPool &&) = delete;

    // 显式的构造函数，需要指定最大连接数
    explicit DBConnectionPool(const std::size_t maxConn);
    // 获取连接，如果连接池已关闭，则返回空
    std::shared_ptr<DBConnection> Acquire(std::chrono::milliseconds timeout);
    // 释放连接
    void Release(std::shared_ptr<DBConnection>);
    // 关闭连接池，释放所有连接
    void CloseAll();

    // 虚析构函数，确保子类析构函数被调用
    virtual ~DBConnectionPool() = default;
    // 纯虚方法 初始化连接池
    virtual void Initialize() = 0;

protected:
    // 纯虚方法 创建新连接
    virtual std::shared_ptr<DBConnection> CreateConnection() = 0;

    // 空闲连接队列
    std::queue<std::shared_ptr<DBConnection>> _idle;
    // 互斥锁，保护连接池的线程安全
    std::mutex _mutex;
    // 条件变量，用于等待连接可用
    std::condition_variable _cond;
    // 最大连接数
    std::size_t _max;
    // 已经创建的连接数
    std::size_t _created;
    // 连接池是否已关闭
    bool _closed;
};

#endif // DBCONNECTIONPOOL_H