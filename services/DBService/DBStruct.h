
#ifndef DBSTRUCT_H
#define DBSTRUCT_H

#include <string>
#include <vector>

#include "../../infra/util/json.hpp"

#pragma region 数据库类型定义

// 数据库类型定义
enum DB_TYPE
{
    DB_TYPE_MYSQL = 1, // MySQL数据库
    DB_TYPE_SQLITE = 2 // SQLite数据库
};

struct DBKey
{
    std::string type;  // mysql / sqlite
    std::string ident; // host:port/db 或 sqlite:path

    bool operator==(const DBKey &rhs) const
    {
        return type == rhs.type && ident == rhs.ident;
    }
};

struct DBKeyHash
{
    std::size_t operator()(const DBKey &k) const
    {
        return std::hash<std::string>()(k.type + "|" + k.ident);
    }
};

#pragma endregion

#pragma region 数据库连接信息结构

struct DBConnConfig
{
    DB_TYPE dbType;       // 数据库类型
    std::string host;     // 主机地址
    uint16_t port;        // 端口号
    std::string user;     // 用户名
    std::string password; // 密码
    std::string database; // 数据库名称
    std::string filePath; // 数据库文件路径（仅用于 SQLite）
};

#pragma endregion

#pragma region 数据库请求信息结构

struct DBRequest
{
    DBKey key;        // 连接信息
    std::string sql;  // SQL语句
    std::string cmd;  // 命令类型
    uint32_t timeout; // 超时时间（毫秒）

    // 默认构造函数
    DBRequest()
    {
        cmd = "execute";
        timeout = 3000; // 默认超时时间为3秒
    }
};

#pragma endregion

#pragma region 数据库响应信息结构

struct DBResult
{
    bool success; // 是否成功

    int errorCode;        // 错误码
    std::string errorMsg; // 错误信息

    // 结果类型
    enum class Type
    {
        NONE,
        RESULT_SET,
        EXEC_RESULT
    } type{Type::NONE};

    // SELECT
    std::vector<std::string> columns;
    std::vector<std::vector<std::string>> rows;

    // 非 SELECT
    int affectedRows;
    int64_t lastInsertId;

    nlohmann::json MakeResultJson() const
    {
        using Type = DBResult::Type;

        nlohmann::json data;

        if (this->type == Type::RESULT_SET)
        {
            data["type"] = "result_set";
            data["columns"] = this->columns;
            data["rows"] = this->rows;
            data["rowCount"] = this->rows.size();
        }
        else if (this->type == Type::EXEC_RESULT)
        {
            data["type"] = "exec_result";
            data["affectedRows"] = this->affectedRows;
            data["lastInsertId"] = this->lastInsertId;
        }
        else
        {
            data["type"] = "ok";
        }

        return data;
    }
};

#pragma endregion

#endif // DBSTRUCT_H