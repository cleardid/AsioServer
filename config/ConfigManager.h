#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <cstdint>
#include <string>

// 配置管理类，用于缓存服务器配置信息，单例模式

class ConfigManager
{

public:
    // 删除拷贝构造函数和赋值运算符
    ConfigManager(const ConfigManager &) = delete;
    ConfigManager &operator=(const ConfigManager &) = delete;
    // 删除移动构造函数和移动赋值运算符
    ConfigManager(ConfigManager &&) = delete;
    ConfigManager &operator=(ConfigManager &&) = delete;
    // 析构函数
    ~ConfigManager() {}

    // 获取端口
    uint16_t GetPort() const { return this->_port; }
    // 获取线程池大小
    uint16_t GetThreadPoolSize() const { return this->_threadPoolSize; }
    // 获取日志路径
    const std::string &GetLogPath() const { return this->_logPath; }

    // 获取单例对象
    static ConfigManager &GetInstance();

    // 对外接口，用于加载配置文件
    void LoadConfig(const std::string &configPath);

private:
    // 私有化构造函数，防止外部创建对象
    ConfigManager() = default;

    // 配置信息，应与 server.json 中的字段对应
    // 服务器端口号
    uint16_t _port;
    // 线程池大小
    uint16_t _threadPoolSize;
    // 日志路径
    std::string _logPath;
};

#endif