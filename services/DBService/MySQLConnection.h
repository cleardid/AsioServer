
#ifndef MYSQLCONNECTION_H
#define MYSQLCONNECTION_H

#include "DBConnection.h"
// #include <mysql/mysql.h>

#include <boost/mysql.hpp>

class MySQLConnection : public DBConnection
{
public:
    // 删除默认构造函数
    MySQLConnection() = delete;
    // 删除拷贝构造函数
    MySQLConnection(const MySQLConnection &) = delete;
    // 删除移动构造函数
    MySQLConnection(MySQLConnection &&) = delete;
    // 删除赋值运算符重载函数
    MySQLConnection &operator=(const MySQLConnection &) = delete;
    // 删除移动赋值运算符重载函数
    MySQLConnection &operator=(MySQLConnection &&) = delete;
    // 显式构造函数，传入连接参数
    explicit MySQLConnection(boost::asio::io_context &ioc,
                             const std::string &host,
                             const uint16_t &port,
                             const std::string &user,
                             const std::string &pwd,
                             const std::string &db);
    ~MySQLConnection();

    bool IsValid() const override;

    boost::asio::awaitable<bool> Execute(const std::string &sql,
                                         DBResult &out) override;

private:
    // 数据库连接对象 初始化为 nullptr
    // MYSQL *_conn;

    // 修改为 boost::mysql::any_connection
    boost::mysql::any_connection _conn;
};

#endif // MYSQLCONNECTION_H