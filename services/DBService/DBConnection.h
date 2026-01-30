
#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include <string>
#include <boost/asio.hpp> // 引入 asio
#include "DBStruct.h"

// 数据库连接基类
class DBConnection
{
public:
    virtual ~DBConnection() = default;
    // 是否有效（连接是否成功）
    virtual bool IsValid() const = 0;
    // 执行SQL语句
    virtual boost::asio::awaitable<bool> Execute(const std::string &sql,
                                                 DBResult &out) = 0;

protected:
    // 是否连接成功
    bool _isConnected = false;
};

#endif // DBCONNECTION_H