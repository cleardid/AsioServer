
#include "DBExecutor.h"
#include "DBConnection.h"
#include "DBConnectionPool.h"
#include "MySQLConnectionPool.h"

#include "../../infra/log/Logger.h"
#include "../../config/ConfigReader.h"

DBExecutor &DBExecutor::GetInstance()
{
    static DBExecutor instance;
    return instance;
}

bool DBExecutor::InitializeFromConfig(const std::string &configPath)
{
    // 读取配置文件
    auto configReader = std::make_shared<ConfigReader>(configPath);
    // 检查是否加载成功
    if (!configReader->IsLoaded())
        return false;

    // 获取原始配置数据
    auto &cfg = configReader->GetRawConfig();
    if (!cfg.contains("databases"))
        return false;

    // 遍历数据库配置，创建连接池
    for (auto &db : cfg["databases"])
    {
        DBKey key;
        std::shared_ptr<DBConnectionPool> pool;

        std::string type = db.at("type").get<std::string>();
        key.type = type;

        if (type == "mysql")
        {
            std::string host = db.at("host");
            uint16_t port = db.at("port");
            std::string user = db.at("user");
            std::string pwd = db.at("password");
            std::string database = db.at("database");

            key.ident = host + ":" + std::to_string(port) + "/" + database;

            std::size_t poolSize = 1;
            // 如果存在 pool 配置，且启用，则读取连接池大小
            if (db.contains("pool") && db["pool"].value("enable", false))
            {
                // 读取连接池大小，默认为4
                poolSize = db["pool"].value("size", 4);
            }

            pool = std::make_shared<MySQLConnectionPool>(
                poolSize, host, port, user, pwd, database);
        }
        else if (type == "sqlite")
        {
            // std::string path = db.at("path");
            // key.ident = path;

            // pool = std::make_shared<SqliteConnectionPool>(path);
        }
        else
        {
            continue;
        }

        std::lock_guard<std::mutex> lock(_mutex);

        LOG_INFO << "Create connection pool for " << key.type << " " << key.ident << std::endl;
        _connPools.emplace(key, pool);
    }

    return true;
}

boost::asio::awaitable<DBResult> DBExecutor::ExecuteRequest(const DBRequest &request)
{
    // 声明结果
    DBResult result;

    // 获取连接池
    std::shared_ptr<DBConnectionPool> pool;
    {
        // 加锁，防止多线程同时访问
        std::lock_guard<std::mutex> lock(this->_mutex);

        // 根据请求类型和标识查找连接池
        auto it = _connPools.find(request.key);
        // LOG_INFO << request.key.type << " " << request.key.ident << std::endl;
        if (it != _connPools.end())
            pool = it->second;
    }

    // 如果连接池不存在，返回失败
    if (!pool)
    {
        LOG_WARN << "Connection pool not found for key." << std::endl;
        result.success = false;
        result.errorMsg = "Connection pool not found";
        co_return result;
    }

    LOG_DEBUG << "DBExecutor: ExecuteRequest for key type = "
              << request.key.type << ", ident = "
              << request.key.ident << std::endl;

    // 如果请求为关闭连接
    if (request.cmd == "close")
    {
        // 关闭连接池并移除
        pool->CloseAll();

        // 加锁
        std::lock_guard<std::mutex> lock(_mutex);
        // 从字典中移除连接池
        this->_connPools.erase(request.key);

        result.success = true;
        co_return result;
    }

    // 从连接池中获取连接
    auto conn = pool->Acquire(std::chrono::milliseconds(request.timeout));
    if (!conn)
    {
        LOG_WARN << "Acquire connection timeout." << std::endl;
        result.success = false;
        result.errorMsg = "Acquire connection timeout";
        co_return result;
    }

    LOG_DEBUG << "ExecuteRequest: " << request.sql << std::endl;

    // 执行请求
    auto res = co_await conn->Execute(request.sql, result);
    result.success = static_cast<bool>(res);
    // 释放连接
    pool->Release(conn);
    // 返回结果
    co_return result;
}

void DBExecutor::Shutdown()
{
    // 加锁
    std::lock_guard<std::mutex> lock(_mutex);
    // 关闭所有连接池
    for (auto &[_, pool] : _connPools)
        pool->CloseAll();
    // 清空字典
    _connPools.clear();
}
