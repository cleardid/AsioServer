
#include "SqliteConnectionPool.h"
#include "SqliteConnection.h"

#include "../../infra/log/Logger.h"

// 最大连接数必须为1 ，因为sqlite不支持多线程
SqliteConnectionPool::SqliteConnectionPool(const std::string &dbPath)
    : DBConnectionPool(1),
      _dbPath(dbPath)
{
    Initialize();
}

void SqliteConnectionPool::Initialize()
{
    try
    {
        auto conn = std::make_shared<SqliteConnection>(this->_dbPath);
        // 安全性检查
        if (conn->IsValid())
        {
            this->_idle.push(conn);
            this->_created = 1;
        }
        else
        {
            // 抛出异常
            throw std::runtime_error("Failed to create SqliteConnection for the pool.");
        }

        LOG_INFO << this->_dbPath << " SqliteConnectionPool initialized with " << this->_created << " connections.\n";
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << e.what() << '\n';
    }
}

std::shared_ptr<DBConnection> SqliteConnectionPool::CreateConnection()
{
    // 创建连接
    auto conn = std::make_shared<SqliteConnection>(this->_dbPath);

    if (!conn->IsValid())
    {
        return nullptr;
    }

    return conn;
}
