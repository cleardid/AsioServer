
#include "ConfigReader.h"

#include "../infra/log/Logger.h"
#include "../infra/util/PathUtils.h"

#include <functional>
#include <memory>

ConfigReader::ConfigReader(const std::string &configPath)
    : _configJson(nullptr),
      _configFilePath(configPath)
{
    // 加载配置文件
    this->_isLoaded = LoadConfig();
}

bool ConfigReader::HasKey(const std::string &key) const
{
    // 如果未加载 或 配置数据为空，则返回false
    if (!this->IsLoaded() || this->_configJson == nullptr)
    {
        if (!this->IsLoaded())
        {
            LOG_WARN << "Config file not loaded: File not found" << '\n';
        }
        else
        {
            LOG_WARN << "Config file not loaded: JSON parsing failed" << '\n';
        }
        return false;
    }

    try
    {
        nlohmann::json::json_pointer ptr("/" + key);
        return this->_configJson->contains(ptr);
    }
    catch (...)
    {
        return false;
    }

    return false;
}

const json &ConfigReader::GetRawConfig() const
{
    static nlohmann::json emptyJson;
    return this->_configJson ? *this->_configJson : emptyJson;
}

void ConfigReader::SetConfigPath(const std::string &newPath)
{
    // 首先检查文件路径是否合理
    if (!ValidateConfigPath(newPath))
    {
        return;
    }

    // 更新配置文件路径
    this->_configFilePath = newPath;
}

bool ConfigReader::ReloadConfig()
{
    return LoadConfig();
}

template <typename T>
inline std::optional<T> ConfigReader::Get(const std::string &key) const
{
    // 如果配置文件未加载，则返回空
    if (!this->IsLoaded() || this->_configJson == nullptr)
    {
        if (!this->IsLoaded())
        {
            LOG_WARN << "Config file not loaded: File not found" << '\n';
        }
        else
        {
            LOG_WARN << "Config file not loaded: JSON parsing failed" << '\n';
        }
        return std::nullopt;
    }

    // 尝试获取配置项
    try
    {
        // 使用指针检查是否存在
        json::json_pointer ptr("/" + key);
        if (!this->_configJson->contains(ptr))
        {
            LOG_WARN << "Config key does not exist: " << key << '\n';
            return std::nullopt;
        }

        // 尝试获取值
        return this->_configJson->at(ptr).get<T>();
    }
    catch (const std::exception &e)
    {
        LOG_WARN << e.what() << '\n';
    }

    return std::optional<T>();
}

std::optional<unsigned int> ConfigReader::GetUInt(const std::string &key) const
{
    return Get<unsigned int>(key);
}

std::optional<int> ConfigReader::GetInt(const std::string &key) const
{
    return Get<int>(key);
}

std::optional<std::string> ConfigReader::GetString(const std::string &key) const
{
    return Get<std::string>(key);
}

std::optional<bool> ConfigReader::GetBool(const std::string &key) const
{
    return Get<bool>(key);
}

std::optional<double> ConfigReader::GetDouble(const std::string &key) const
{
    return Get<double>(key);
}

bool ConfigReader::LoadConfig()
{
    try
    {
        // 验证配置文件路径
        if (!ValidateConfigPath(this->_configFilePath))
        {
            LOG_WARN << "Config file path is invalid: " << this->_configFilePath << std::endl;
            return false;
        }

        fs::path configPath(this->_configFilePath);

        // 如果是相对路径 → 基于 exe 目录
        if (configPath.is_relative())
        {
            fs::path exeDir = util::GetExecutableDir();
            configPath = exeDir / configPath;
        }

        // 规范化路径（不要求真实存在）
        configPath = fs::weakly_canonical(configPath);

        // 检查文件是否存在
        if (!fs::exists(configPath))
        {
            // 输出当前执行目录
            LOG_WARN << "Current working directory: " << fs::current_path() << std::endl;
            LOG_WARN << "Config file does not exist: " << configPath << std::endl;
            return false;
        }

        // 打开配置文件
        auto file = std::ifstream(configPath, std::ios::in);
        if (!file.is_open())
        {
            LOG_WARN << "Failed to open config file: " << configPath << std::endl;
            return false;
        }

        // 创建 JSON 对象
        auto j = std::make_shared<json>();
        // 从文件中读取 JSON 数据
        file >> *j;

        // 盼空
        if (j->empty())
        {
            LOG_WARN << "Config file is empty: " << configPath << std::endl;
            return false;
        }

        // 更新成员
        this->_configFilePath = configPath.string();
        this->_configJson = std::move(j);

        return true;
    }
    catch (const std::exception &e)
    {
        LOG_WARN << "LoadConfig exception: " << e.what() << '\n';
        return false;
    }

    return false;
}

bool ConfigReader::ValidateConfigPath(const std::string &configPath) const
{
    // 检查配置文件路径是否为空
    if (configPath.empty())
        return false;

    // 检查文件扩展名是否为 .json
    fs::path p(configPath);

    if (!p.has_extension() || p.extension() != ".json")
    {
        LOG_WARN << "Config file extension is not .json: " << configPath << '\n';
        return false;
    }

    return true;
}
