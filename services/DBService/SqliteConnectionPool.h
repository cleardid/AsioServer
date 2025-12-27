
#ifndef SQLITECONNECTIONPOOL_H
#define SQLITECONNECTIONPOOL_H

#include "DBConnectionPool.h"

class SqliteConnectionPool : public DBConnectionPool
{
public:
    // 删除默认构造函数
    SqliteConnectionPool() = delete;
    // 删除拷贝构造函数
    SqliteConnectionPool(const SqliteConnectionPool &) = delete;
    // 删除移动构造函数
    SqliteConnectionPool(SqliteConnectionPool &&) = delete;
    // 删除拷贝赋值运算符
    SqliteConnectionPool &operator=(const SqliteConnectionPool &) = delete;
    // 删除移动赋值运算符
    SqliteConnectionPool &operator=(SqliteConnectionPool &&) = delete;

    // 显式指定构造函数
    explicit SqliteConnectionPool(const std::string &dbPath);

    // 析构函数
    ~SqliteConnectionPool() override = default;

    void Initialize() override;

    std::shared_ptr<DBConnection> CreateConnection() override;

private:
    std::string _dbPath;
};

#endif // SQLITECONNECTIONPOOL_H