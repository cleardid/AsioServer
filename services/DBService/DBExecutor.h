
#ifndef DBEXECUTOR_H
#define DBEXECUTOR_H

#include "DBStruct.h"

#include <unordered_map>
#include <mutex>
#include <boost/asio.hpp>

// 前置声明
class DBConnectionPool;

class DBExecutor
{
public:
    // 删除拷贝构造函数
    DBExecutor(const DBExecutor &) = delete;
    // 删除移动构造函数
    DBExecutor(DBExecutor &&) = delete;
    // 删除赋值运算符重载函数
    DBExecutor &operator=(const DBExecutor &) = delete;
    // 删除移动赋值运算符重载函数
    DBExecutor &operator=(DBExecutor &&) = delete;

    // 析构
    ~DBExecutor() = default;

    // 静态方法 获取单例
    static DBExecutor &GetInstance();

    // 从配置文件中读取数据库信息，创建 Mysql / SQLite 连接池
    // 文件路径应以 ConfigReader.h 所在的路径为准
    bool InitializeFromConfig(const std::string &configPath = "../config/database.json");
    // 协程 执行数据库请求 返回结果
    boost::asio::awaitable<DBResult> ExecuteRequest(const DBRequest &request);

    // 关闭所有连接
    void Shutdown();

private:
    DBExecutor() = default;

    // 连接池字典
    std::unordered_map<DBKey, std::shared_ptr<DBConnectionPool>, DBKeyHash> _connPools;
    // 互斥锁 保护连接池字典的线程安全
    std::mutex _mutex;
};

#endif // DBEXECUTOR_H