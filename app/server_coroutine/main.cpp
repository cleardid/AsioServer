
#include <iostream>
#include <boost/asio.hpp>
#include <mysql/mysql.h>

#include "../../config/ConfigReader.h"

#include "../../core/server/CServer.h"
#include "../../core/session/AsioIOServicePool.h"

#include "../../infra/log/Logger.h"

#include "../../services/IService.h"
#include "../../services/ServiceManager.h"
#include "../../services/HelloService/HelloService.h"
#include "../../services/DBService/DBExecutor.h"

// 初始化日志系统
void InitLog(const std::string &configPath)
{
    auto &logger = Logger::GetInstance();

    // 1. 添加控制台+文件输出器
    logger.AddAppender(std::make_unique<ConsoleAppender>());
    // 读取日志输出目录
    // 创建配置文件读取器
    auto configReader = std::make_shared<ConfigReader>(configPath);
    auto logFile = configReader->GetString("log_path").value_or("./server.log");
    logger.AddAppender(std::make_unique<FileAppender>(logFile));
    // 释放配置文件读取器
    configReader.reset();

    // 2. 启用异步日志（队列容量10000）
    logger.SetAsyncMode(true, 10000);

    // 3. 设置日志级别为DEBUG
    logger.SetLevel(LogLevel::DEBUG);
}

// 从配置文件中读取端口配置
uint16_t GetPortFromConfig(const std::string &configPath)
{
    // 定义默认端口
    uint16_t port = 11111;

    // 创建配置文件读取器
    auto configReader = std::make_shared<ConfigReader>(configPath);
    // 读取端口号
    auto setPort = configReader->GetUInt("port").value_or(11111);
    // 释放配置文件读取器
    configReader.reset();

    if (setPort > 65535 || setPort < 1024)
    {
        LOG_ERROR << "Invalid port number: " << setPort << std::endl;
        return port;
    }

    LOG_INFO << "Get Config Server port: " << setPort << std::endl;
    return setPort;
}

// mysql测试
void MysqlTest()
{
    // MySQL 连接信息（替换为你的配置）
    const std::string HOST = "localhost";
    const std::string USER = "root";
    const std::string PASSWORD = "123456"; // 替换为你重置的 MySQL 密码
    const std::string DB_NAME = "mysql";   // 系统默认数据库，无需创建
    const unsigned int PORT = 3306;

    // 1. 初始化 MySQL 连接句柄
    MYSQL *conn = mysql_init(nullptr);
    if (conn == nullptr)
    {
        LOG_ERROR << "MySQL 句柄初始化失败！错误信息：" << mysql_error(conn) << std::endl;
    }

    // 2. 设置字符集（避免中文乱码）
    mysql_options(conn, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    // 3. 连接 MySQL 服务器
    if (mysql_real_connect(conn, HOST.c_str(), USER.c_str(),
                           PASSWORD.c_str(), DB_NAME.c_str(), PORT, nullptr, 0) == nullptr)
    {
        LOG_ERROR << "MySQL 连接失败！错误信息：" << mysql_error(conn) << std::endl;
        mysql_close(conn);
    }

    LOG_INFO << "✅ MySQL 8.0 连接成功！" << std::endl;

    // 4. 简单测试：执行查询（查看 MySQL 版本）
    const char *sql = "SELECT VERSION();";
    if (mysql_query(conn, sql) != 0)
    {
        LOG_ERROR << "查询失败！错误信息：" << mysql_error(conn) << std::endl;
        mysql_close(conn);
    }

    // 5. 处理查询结果
    MYSQL_RES *res = mysql_store_result(conn);
    if (res == nullptr)
    {
        LOG_ERROR << "获取结果集失败！错误信息：" << mysql_error(conn) << std::endl;
        mysql_close(conn);
    }

    // 读取结果
    MYSQL_ROW row = mysql_fetch_row(res);
    if (row != nullptr)
    {
        LOG_INFO << "📌 MySQL 服务器版本：" << row[0] << std::endl;
    }

    // 6. 释放资源
    mysql_free_result(res);
    mysql_close(conn);
}

// 注册服务
void RegisterServices()
{
    ServiceManager::GetInstance().RegisterService(std::make_shared<HelloService>());
}

int main()
{

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    try
    {
        // 初始化日志系统
        InitLog("../config/server.json");
        // 读取数据库配置
        DBExecutor::GetInstance().InitializeFromConfig("../config/database.json");
        // 注册服务
        RegisterServices();

        // LOG_INFO << "Server starting at " << __TIME__ << "\n";
        // MysqlTest();
        // LOG_INFO << "MySQL test completed.\n";

        // 端口
        const uint16_t port = GetPortFromConfig("../config/server.json");
        // 获取线程池
        auto &pool = AsioIOServicePool::GetInstance();
        // 获取连接的上下文
        boost::asio::io_context ioc;
        // 添加信号量，用于退出
        boost::asio::signal_set signals(ioc, SIGINT, SIGTERM);
        // 异步等待
        signals.async_wait([&](auto, auto)
                           {
                                ioc.stop();
                                pool.Stop(); });
        // 声明服务
        CServer server(ioc, port);
        // 启动服务
        ioc.run();
    }
    catch (const std::exception &e)
    {
        LOG_FATAL << e.what() << '\n';
    }

    LOG_FATAL << "Server shutdown at " << __TIME__ << "\n";

    return 0;
}