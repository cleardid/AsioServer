
#include "MySQLConnectionPool.h"
#include "MySQLConnection.h"

#include "../../infra/log/Logger.h"
#include "../../core/session/AsioIOServicePool.h"

MySQLConnectionPool::MySQLConnectionPool(const std::size_t maxConn, const std::string &host, const uint16_t &port, const std::string &user, const std::string &pwd, const std::string &db)
    : DBConnectionPool(maxConn),
      _host(host),
      _port(port),
      _user(user),
      _pwd(pwd),
      _db(db)
{
    // 初始化连接池
    Initialize();
}

void MySQLConnectionPool::Initialize()
{
    try
    {
        // 加锁
        std::lock_guard<std::mutex> lock(this->_mutex);

        // 预创建一半的连接
        for (std::size_t i = 0; i < this->_max / 2; ++i)
        {
            // 从上下文池中获取连接
            auto &ioc = AsioIOServicePool::GetInstance().GetIOServive();
            // 创建连接
            auto conn = std::make_shared<MySQLConnection>(ioc, this->_host, this->_port, this->_user, this->_pwd, this->_db);
            if (conn->IsValid())
            {
                // 加入空闲连接队列
                this->_idle.push(conn);
            }
            else
            {
                // 抛出异常
                throw std::runtime_error("Failed to create MySQLConnection for the pool.");
            }
        }

        // 增加已创建连接数
        this->_created = this->_idle.size();

        LOG_INFO << this->_db << " MySQLConnectionPool initialized with "
                 << this->_created << " connections." << std::endl;
    }
    catch (const std::exception &e)
    {
        LOG_ERROR << e.what() << '\n';
    }
}

std::shared_ptr<DBConnection> MySQLConnectionPool::CreateConnection()
{
    // 从上下文池中获取连接
    auto &ioc = AsioIOServicePool::GetInstance().GetIOServive();

    auto conn = std::make_shared<MySQLConnection>(ioc, _host, _port, _user, _pwd, _db);
    // 安全性检查
    if (!conn->IsValid())
        return nullptr;

    return conn;
}
