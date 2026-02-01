#include "ConfigManager.h"
#include "ConfigReader.h"

ConfigManager &ConfigManager::GetInstance()
{
    static ConfigManager instance;
    return instance;
}

void ConfigManager::LoadConfig(const std::string &configPath)
{
    // 创建配置文件读取器
    auto configReader = std::make_shared<ConfigReader>(configPath);
    // 获取配置文件中的配置项
    this->_port = configReader->GetInt("port").value_or(19998);
    this->_threadPoolSize = configReader->GetInt("thread_pool_size").value_or(2);
    this->_logPath = configReader->GetString("log_path").value_or("./server.log");
    // 关闭配置文件读取器
    configReader.reset();
}