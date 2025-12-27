
#ifndef DBCONNECTION_H
#define DBCONNECTION_H

#include <string>
#include "DBStruct.h"

// 数据库连接基类
class DBConnection
{
public:
    virtual ~DBConnection() = default;
    // 是否有效（连接是否成功）
    virtual bool IsValid() const = 0;
    // 执行SQL语句
    virtual bool Execute(const std::string &sql,
                         DBResult &out) = 0;

protected:
    // 是否连接成功
    bool _isConnected;
};

#endif // DBCONNECTION_H