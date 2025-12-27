
#include "MySQLConnection.h"
#include "../../infra/log/Logger.h"

#include <sstream>

MySQLConnection::MySQLConnection(const std::string &host,
                                 const uint16_t &port,
                                 const std::string &user,
                                 const std::string &pwd,
                                 const std::string &db)
{
    // 初始化 MySQL 连接对象
    this->_conn = mysql_init(nullptr);
    // 检查初始化是否成功
    if (!this->_conn)
    {
        return;
    }
    // 连接 MySQL 数据库
    if (!mysql_real_connect(this->_conn, host.c_str(), user.c_str(), pwd.c_str(),
                            db.c_str(), port, nullptr, 0))
    {
        mysql_close(this->_conn);
        this->_conn = nullptr;
        this->_isConnected = false;
        return;
    }

    this->_isConnected = true;
    LOG_INFO << "MySQL Connection Success -> " << host << ":" << port << "/" << db << std::endl;
}

MySQLConnection::~MySQLConnection()
{
    if (this->_conn)
    {
        mysql_close(this->_conn);
        this->_conn = nullptr;
    }
}

bool MySQLConnection::IsValid() const
{
    return this->_isConnected;
}

bool MySQLConnection::Execute(const std::string &sql, DBResult &out)
{
    // 安全性检查
    if (!this->_isConnected)
    {
        out.errorMsg = "MySQL Connection Not Valid";
        return false;
    }

    // 执行 SQL 语句
    if (mysql_query(_conn, sql.c_str()))
    {
        out.errorMsg = mysql_error(_conn);
        LOG_ERROR << "MySQL Execute Error: " << out.errorMsg << std::endl;
        return false;
    }

    // 获取结果集
    MYSQL_RES *res = mysql_store_result(_conn);

    // 如果没有结果集，直接返回
    // 非 SELECT 语句（INSERT / UPDATE / DELETE）
    if (!res)
    {
        // 非 SELECT
        out.type = DBResult::Type::EXEC_RESULT;
        out.affectedRows = mysql_affected_rows(_conn);
        out.lastInsertId = mysql_insert_id(_conn);
        return true;
    }

    // 获取结果集的列数和字段信息
    int colCount = mysql_num_fields(res);
    MYSQL_FIELD *fields = mysql_fetch_fields(res);
    // 标志结果为 SELECT 结果集
    out.type = DBResult::Type::RESULT_SET;
    // 获取列名
    for (int i = 0; i < colCount; ++i)
        out.columns.emplace_back(fields[i].name);

    // 获取行数据
    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res)))
    {
        std::vector<std::string> r;
        for (int i = 0; i < colCount; ++i)
            r.emplace_back(row[i] ? row[i] : "");
        out.rows.emplace_back(std::move(r));
    }

    // 释放结果集
    mysql_free_result(res);
    return true;
}
