
#include "Logger.h"
#include "../util/PathUtils.h"

std::string PatternLayout::Format(const LogEvent &event)
{
    std::ostringstream oss;

    // 1. 时间戳（yyyy-MM-dd HH:mm:ss.SSS）
    auto time_t = std::chrono::system_clock::to_time_t(event._time);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(event._time.time_since_epoch()) % 1000;
    oss << std::put_time(std::localtime(&time_t), "[%Y-%m-%d %H:%M:%S.")
        << std::setfill('0') << std::setw(3) << ms.count() << "] ";

    // // 2. 线程ID
    // oss << "[" << event.thread_id << "] ";

    // 3. 日志级别
    oss << "[" << LogLevelToString(event._level) << "] ";

    // 4. 文件+行号
    oss << "[" << event._file << ":" << event._line << "] - ";

    // 5. 日志消息
    oss << event._msg;

    return oss.str();
}

// 设置自定义格式化器
void LogAppender::SetLayout(std::unique_ptr<LogLayout> layout)
{
    if (layout)
        this->_layout = std::move(layout);
}

void ConsoleAppender::Append(const LogEvent &event)
{
    // 格式化后输出到控制台
    std::cout << this->_layout->Format(event);
}

// 构造函数：初始化文件路径、滚动阈值、最大备份数
FileAppender::FileAppender(const std::string &file_path, size_t max_size, int max_backup)
    : _filePath(file_path), _maxFileSize(max_size), _maxBackup(max_backup), _currentSize(0)
{
    // 初始化打开文件（追加模式）
    OpenFile();
}

void FileAppender::Append(const LogEvent &event)
{
    std::lock_guard<std::mutex> lock(this->_fileMutex); // 文件写入锁（局部锁，避免全局锁粒度太大）

    // 检查是否需要滚动文件
    if (this->_currentSize >= this->_maxFileSize)
    {
        RollFile();
    }

    // 格式化并写入文件
    std::string log_str = this->_layout->Format(event);
    this->_fileStream << log_str;
    this->_fileStream.flush(); // 立即刷盘，避免缓冲区丢失

    // 更新当前文件大小
    this->_currentSize += log_str.size();
}

// 打开日志文件
void FileAppender::OpenFile()
{
    try
    {
        fs::path logPath(this->_filePath);

        // 相对路径 → 基于可执行文件目录
        if (logPath.is_relative())
        {
            fs::path exeDir = util::GetExecutableDir();
            logPath = exeDir / logPath;
        }

        // 规范化路径
        logPath = fs::weakly_canonical(logPath);

        fs::path dirPath = logPath.parent_path();

        LOG_INFO << "Opening log file: " << logPath << '\n';
        LOG_INFO << "Log directory: " << dirPath << '\n';

        // 创建日志目录
        if (!dirPath.empty() && !fs::exists(dirPath))
        {
            fs::create_directories(dirPath);
            LOG_INFO << "Created log directory: " << dirPath << '\n';
        }

        // 打开文件（追加 + 二进制）
        this->_fileStream.open(
            logPath,
            std::ios::out | std::ios::app | std::ios::binary);

        if (!this->_fileStream.is_open())
        {
            throw std::runtime_error(
                "Failed to open log file: " + logPath.string());
        }

        // 获取当前文件大小
        this->_fileStream.seekp(0, std::ios::end);      // 移动到文件末尾
        this->_currentSize = this->_fileStream.tellp(); // 获取当前位置（即文件大小）

        // 更新文件路径
        this->_filePath = logPath.string();
    }
    catch (const std::exception &e)
    {
        std::cerr << e.what() << '\n';
    }
}

// 滚动日志文件（xxx.log → xxx.log.1，xxx.log.1 → xxx.log.2...）
void FileAppender::RollFile()
{
    this->_fileStream.close();

    // 重命名旧备份文件
    for (int i = this->_maxBackup - 1; i > 0; --i)
    {
        fs::path src = this->_filePath + "." + std::to_string(i);
        fs::path dst = this->_filePath + "." + std::to_string(i + 1);
        if (fs::exists(src))
        {
            fs::rename(src, dst);
        }
    }

    // 重命名当前文件为xxx.log.1
    if (fs::exists(this->_filePath))
    {
        fs::rename(this->_filePath, this->_filePath + ".1");
    }

    // 重新打开新文件
    OpenFile();
}

// 新增：设置异步模式
void Logger::SetAsyncMode(bool async, size_t queue_size)
{
    // 加锁
    std::lock_guard<std::mutex> lock(this->_mutex);
    // 更新同步状态
    this->_isAsync = async;
    // 根据同步状态调整日志处理方式
    if (this->_isAsync)
    {
        // 初始化异步队列和日志线程
        this->_logQue = std::make_unique<SafeQueue<std::shared_ptr<LogEvent>>>(queue_size);
        // 启动日志消费线程
        this->_logThread = std::thread(&Logger::AsyncLogThread, this);
    }
    else
    {
        // 同步模式：停止异步线程，处理剩余日志
        if (this->_logQue)
        {
            this->_logQue->Stop();
            if (this->_logThread.joinable())
            {
                this->_logThread.join();
            }
            this->_logQue.reset();
        }
    }
}

// 添加输出器（支持多输出器：控制台+文件）
void Logger::AddAppender(std::unique_ptr<LogAppender> appender)
{
    std::lock_guard<std::mutex> lock(this->_mutex);
    this->_appenders.push_back(std::move(appender));
}

// 设置全局日志级别（低于该级别的日志不输出）
void Logger::SetLevel(LogLevel level)
{
    std::lock_guard<std::mutex> lock(this->_mutex);
    this->_level = level;
}

// 析构函数：优雅停止异步线程，处理剩余日志
Logger::~Logger()
{
    if (this->_isAsync && this->_logQue)
    {
        this->_logQue->Stop();
        if (this->_logThread.joinable())
        {
            this->_logThread.join();
        }
    }
}

// 异步日志消费线程入口
void Logger::AsyncLogThread()
{
    std::shared_ptr<LogEvent> event;
    while (this->_logQue->Pop(event))
    {
        // 消费队列中的日志事件，分发到输出器
        DispatchLog(event);
    }

    // 处理队列剩余日志 可能在输出的时候又添加了新的日志
    while (this->_logQue->Size() > 0)
    {
        if (this->_logQue->Pop(event))
        {
            DispatchLog(event);
        }
    }
}

// 分发日志到所有输出器（抽离为独立方法）
void Logger::DispatchLog(const std::shared_ptr<LogEvent> &event)
{
    // 加锁
    std::lock_guard<std::mutex> lock(this->_mutex);
    // 遍历所有输出器，逐个追加日志
    for (auto &appender : this->_appenders)
    {
        appender->Append(*event);
    }
}

void Logger::log(LogLevel level, const char *file, int line, const std::string &msg)
{
    // 1. 级别过滤
    if (level < this->_level)
        return;

    // 2. 封装日志事件（直接用格式化后的msg，无需再解析参数）
    auto event = std::make_shared<LogEvent>(level, file, line, msg);

    // 3. 异步/同步分发（原逻辑完全不变）
    if (this->_isAsync && this->_logQue)
    {
        this->_logQue->Push(event);
    }
    else
    {
        DispatchLog(event);
    }
}