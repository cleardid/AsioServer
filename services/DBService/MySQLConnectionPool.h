
#ifndef MYSQLCONNECTIONPOOL_H
#define MYSQLCONNECTIONPOOL_H

#include "DBConnectionPool.h"

class MySQLConnectionPool : public DBConnectionPool
{
public:
    // 删除默认构造函数
    MySQLConnectionPool() = delete;
    // 删除拷贝构造函数
    MySQLConnectionPool(const MySQLConnectionPool &) = delete;
    // 删除移动构造函数
    MySQLConnectionPool(MySQLConnectionPool &&) = delete;
    // 删除赋值运算符重载函数
    MySQLConnectionPool &operator=(const MySQLConnectionPool &) = delete;
    // 删除移动赋值运算符重载函数
    MySQLConnectionPool &operator=(MySQLConnectionPool &&) = delete;

    explicit MySQLConnectionPool(const std::size_t maxConn,
                                 const std::string &host,
                                 const uint16_t &port,
                                 const std::string &user,
                                 const std::string &pwd,
                                 const std::string &db);
    ~MySQLConnectionPool() override = default;

    void Initialize() override;

    std::shared_ptr<DBConnection> CreateConnection() override;

private:
    std::string _host;
    uint16_t _port;
    std::string _user;
    std::string _pwd;
    std::string _db;
};

#endif // MYSQLCONNECTIONPOOL_H