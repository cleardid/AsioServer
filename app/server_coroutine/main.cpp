
#include <iostream>
#include <boost/asio.hpp>
#include <mysql/mysql.h>

#include "../../config/ConfigManager.h"

#include "../../core/server/CServer.h"
#include "../../core/session/AsioIOServicePool.h"

#include "../../infra/log/Logger.h"

#include "../../services/IService.h"
#include "../../services/ServiceManager.h"
#include "../../services/HelloService/HelloService.h"
#include "../../services/DBService/DBExecutor.h"

// 初始化日志系统
void InitLog()
{
    auto &logger = Logger::GetInstance();

    // 1. 添加控制台+文件输出器
    logger.AddAppender(std::make_unique<ConsoleAppender>());
    // 读取日志输出目录
    // 创建配置文件读取器
    auto logFile = ConfigManager::GetInstance().GetLogPath();
    logger.AddAppender(std::make_unique<FileAppender>(logFile));

    // 2. 启用异步日志（队列容量10000）
    logger.SetAsyncMode(true, 10000);

    // 3. 设置日志级别为DEBUG
    logger.SetLevel(LogLevel::DEBUG);
}

// 从配置文件中读取端口配置
uint16_t GetPortFromConfig()
{
    // 获取端口号
    auto port = ConfigManager::GetInstance().GetPort();

    // 检查端口号是否合法
    if (port > 65535 || port < 1024)
    {
        LOG_ERROR << "Invalid port number: " << port << std::endl;
        // 使用默认端口
        return 19998;
    }

    LOG_INFO << "Get Config Server port: " << port << std::endl;
    return port;
}

// 跨平台获取相对路径对应的绝对路径
std::string GetAbsolutePath(const char *exePath, const std::string &relativePath)
{
    // 边界检查：exePath为空则返回空字符串
    if (exePath == nullptr || *exePath == '\0')
    {
        throw std::invalid_argument("exePath is null or empty");
    }

    // 1. 将可执行文件路径转为filesystem路径对象，自动识别系统分隔符
    fs::path exePathObj(exePath);
    // 2. 获取可执行文件所在目录（自动处理绝对/相对路径）
    fs::path exeDir = exePathObj.parent_path();
    // 3. 拼接相对路径并规范化（自动解析../、./，统一分隔符）
    fs::path absolutePath = fs::canonical(exeDir / relativePath);

    // 转为字符串返回（自动适配系统分隔符）
    return absolutePath.string();
}

// 注册服务
void RegisterServices()
{
    ServiceManager::GetInstance().RegisterService(std::make_shared<HelloService>());
}

int main(int argc, char *argv[])
{

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    try
    {
        // 获取 exe 路径并计算 config 绝对路径
        std::string configPath = GetAbsolutePath(argv[0], "../config/server.json");
        // 加载配置文件
        ConfigManager::GetInstance().LoadConfig(configPath);

        // 初始化日志系统
        InitLog();
        // 读取数据库配置
        DBExecutor::GetInstance().InitializeFromConfig("../config/database.json");
        // 注册服务
        RegisterServices();

        // 端口
        const uint16_t port = GetPortFromConfig();
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