
#ifndef CONST_H
#define CONST_H

#include <string>
#include <cstddef>

const std::size_t MAX_RECVQUE_LEN = 10000; // 最大接收队列长度
const std::size_t MAX_SENDQUE_LEN = 1000;  // 最大发送队列长度

// ASIO类型枚举
enum ASIO_TYPE
{
    COROUTINE = 0, // 协程
    ASYNC = 1      // 异步线程
};

// 服务枚举
enum SERVICE_TYPE
{
    SERVICE_HELLO = 1,         // hello服务
    SERVICE_DB = 2,            // 数据库服务
    SERVICE_COMMUNICATION = 3, // 通信服务
};

// hello 服务命令枚举
enum HELLO_CMD
{
    HELLO_CMD_TEST = 1, // 测试命令
};

// 数据库服务命令枚举
enum DB_CMD
{
    DB_EXECUTE = 1, // 执行命令
    DB_CLOSE = 2,   // 关闭连接
};

// 通信服务命令枚举
enum COMMUNICATION_CMD
{
    COMMUINICATION_CREATE = 1, // 创建连接
    COMMUINICATION_CLOSE = 2,  // 关闭连接
    COMMUINICATION_SEND = 3,   // 发送数据
    COMMUINICATION_SHOW = 4,   // 显示连接信息
};

#pragma region 日志相关枚举及方法

// 日志等级枚举
enum class LogLevel
{
    UNKNOWN = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    MYERROR = 4,
    FATAL = 5
};

// 级别转字符串（输出用）
inline std::string LogLevelToString(LogLevel level)
{
    switch (level)
    {
    case LogLevel::UNKNOWN:
        return "UNKNOWN";
    case LogLevel::DEBUG:
        return "DEBUG";
    case LogLevel::INFO:
        return "INFO ";
    case LogLevel::WARN:
        return "WARN ";
    case LogLevel::MYERROR:
        return "ERROR";
    case LogLevel::FATAL:
        return "FATAL";
    default:
        return "UNKNOWN";
    }
}

// 字符串转级别（可选，用于配置）
inline LogLevel StringToLogLevel(const std::string &str)
{
    if (str == "UNKNOWN")
        return LogLevel::UNKNOWN;
    if (str == "DEBUG")
        return LogLevel::DEBUG;
    if (str == "INFO")
        return LogLevel::INFO;
    if (str == "WARN")
        return LogLevel::WARN;
    if (str == "ERROR")
        return LogLevel::MYERROR;
    if (str == "FATAL")
        return LogLevel::FATAL;
    return LogLevel::INFO; // 默认
}

#pragma endregion

#endif // CONST_H