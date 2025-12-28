
#ifndef LOGGER_H
#define LOGGER_H

#include <chrono>
#include <thread>
#include <string>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <memory>
#include <fstream>
#include <filesystem>
#include <mutex>
#include <vector>
#include <cstdarg>
#include <format> // C++20

#include "../../core/common/Const.h"
#include "SafeQueue.h"

// 前置声明
class LogStream;

#define LOG_DEBUG LogStream(LogLevel::DEBUG, __FILE__, __LINE__)
#define LOG_INFO LogStream(LogLevel::INFO, __FILE__, __LINE__)
#define LOG_WARN LogStream(LogLevel::WARN, __FILE__, __LINE__)
#define LOG_ERROR LogStream(LogLevel::MYERROR, __FILE__, __LINE__)
#define LOG_FATAL LogStream(LogLevel::FATAL, __FILE__, __LINE__)

// 日志事件
struct LogEvent
{
    // 时间戳（毫秒级）
    std::chrono::system_clock::time_point time;
    // 线程ID
    uint64_t thread_id;
    // 日志级别
    LogLevel level;
    // 文件名（__FILE__宏）
    const char *file;
    // 行号（__LINE__宏）
    int line;
    // 日志消息
    std::string msg;

    // 构造函数：初始化基础信息（时间、线程ID）
    LogEvent(LogLevel level, const char *file, int line, const std::string &msg)
        : level(level), file(file), line(line), msg(msg), time(std::chrono::system_clock::now())
    {
        // // 将std::thread::id转换为uint64_t（跨平台兼容）
        // std::hash<std::thread::id> hasher;
        // thread_id = hasher(std::this_thread::get_id());
    }
};

// 格式输出器
class LogLayout
{
public:
    virtual ~LogLayout() = default;
    // 核心方法：格式化日志事件
    virtual std::string Format(const LogEvent &event) = 0;
};

// 参考LOG4J的默认格式实现
class PatternLayout : public LogLayout
{
public:
    // 格式化方法：将日志事件格式化为字符串
    std::string Format(const LogEvent &event) override;
};

// 抽象基类 LogAppender
class LogAppender
{
public:
    LogAppender() : _layout(std::make_unique<PatternLayout>()) {}
    virtual ~LogAppender() = default;

    // 设置自定义格式化器
    void SetLayout(std::unique_ptr<LogLayout> layout);
    // 纯虚方法 输出日志
    virtual void Append(const LogEvent &event) = 0;

protected:
    // 格式化器
    std::unique_ptr<LogLayout> _layout;
};

// 控制台输出
class ConsoleAppender : public LogAppender
{
public:
    // 重写 Append 方法
    void Append(const LogEvent &event) override;
};

namespace fs = std::filesystem;
class FileAppender : public LogAppender
{
public:
    // 构造函数：初始化文件路径、滚动阈值、最大备份数
    FileAppender(const std::string &file_path, size_t max_size = 100 * 1024 * 1024, int max_backup = 10);

    // 重写 Append 方法
    void Append(const LogEvent &event) override;

private:
    // 打开日志文件
    void OpenFile();
    // 滚动日志文件（xxx.log → xxx.log.1，xxx.log.1 → xxx.log.2...）
    void RollFile();

private:
    std::string _filePath;     // 日志文件路径
    size_t _maxFileSize;       // 文件大小阈值（默认100MB）
    int _maxBackup;            // 最大备份文件数
    size_t _currentSize;       // 当前文件大小
    std::ofstream _fileStream; // 文件输出流
    std::mutex _fileMutex;     // 文件写入互斥锁（局部锁）
};

class Logger
{
public:
    // 单例获取（Meyers单例，C++11+线程安全）
    static Logger &GetInstance()
    {
        static Logger instance;
        return instance;
    }

    // 禁止拷贝/移动
    Logger(const Logger &) = delete;
    Logger &operator=(const Logger &) = delete;
    Logger(Logger &&) = delete;
    Logger &operator=(Logger &&) = delete;

    // 设置异步模式（默认同步）
    void SetAsyncMode(bool async = true, size_t queue_size = 10000);
    // 添加输出器
    void AddAppender(std::unique_ptr<LogAppender> appender);
    // 设置全局日志级别（低于该级别的日志不输出）
    void SetLevel(LogLevel level);

    // 私有成员方法 用于处理日志输出
    void log(LogLevel level, const char *file, int line, const std::string &msg);

    // 析构函数：优雅停止异步线程，处理剩余日志
    ~Logger();

private:
    Logger() : _level(LogLevel::INFO), _isAsync(false) {}
    // 异步日志消费线程入口
    void AsyncLogThread();
    // 分发日志到所有输出器
    void DispatchLog(const std::shared_ptr<LogEvent> &event);

    LogLevel _level;                                               // 全局日志级别
    std::vector<std::unique_ptr<LogAppender>> _appenders;          // 输出器列表
    std::mutex _mutex;                                             // 全局互斥锁
    bool _isAsync;                                                 // 是否异步模式
    std::unique_ptr<SafeQueue<std::shared_ptr<LogEvent>>> _logQue; // 异步日志队列
    std::thread _logThread;                                        // 异步日志消费线程
};

// 日志流类：负责拼接日志内容，析构时输出日志
class LogStream
{
public:
    // 构造函数：绑定日志级别、文件、行号，关联全局 Logger
    LogStream(LogLevel level, const char *file, int line)
        : _level(level), _file(file), _line(line), _logger(Logger::GetInstance()) {}

    // 析构函数：核心！临时对象析构时，触发日志输出
    ~LogStream()
    {
        // 将拼接好的内容转为字符串，调用 Logger 的核心方法
        this->_logger.log(this->_level, this->_file, this->_line, this->_oss.str());
    }

    // 重载 << 运算符：支持所有可流式输出的类型（模板重载）
    template <typename T>
    LogStream &operator<<(const T &value)
    {
        this->_oss << value; // 拼接内容到内部的 ostringstream
        return *this;        // 返回自身，支持链式调用（<< 连续拼接）
    }

    // 特殊重载：支持 std::endl / std::flush 等操纵符
    LogStream &operator<<(std::ostream &(*manip)(std::ostream &))
    {
        this->_oss << manip; // 处理 endl/flush 等
        return *this;
    }

private:
    LogLevel _level;         // 日志级别
    const char *_file;       // 文件名（__FILE__）
    int _line;               // 行号（__LINE__）
    std::ostringstream _oss; // 核心：拼接日志内容
    Logger &_logger;         // 关联全局 Logger 实例
};

#endif // LOGGER_H