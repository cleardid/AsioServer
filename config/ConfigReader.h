
#ifndef CONFIGREADER_H
#define CONFIGREADER_H

#include "../infra/util/json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

class ConfigReader
{
public:
    // 构造函数：传入配置文件路径
    explicit ConfigReader(const std::string &configPath = "../config/server.json");

    // 析构函数
    ~ConfigReader() = default;

    // 禁止拷贝和赋值
    ConfigReader(const ConfigReader &) = delete;
    ConfigReader &operator=(const ConfigReader &) = delete;

    // 允许移动
    ConfigReader(ConfigReader &&) = default;
    ConfigReader &operator=(ConfigReader &&) = default;

    // 对外接口：

    // 检查配置项是否存在
    bool HasKey(const std::string &key) const;
    // 获取整个配置JSON对象（只读）
    const json &GetRawConfig() const;
    // 设置配置文件路径
    void SetConfigPath(const std::string &newPath);
    // 重新加载配置文件
    bool ReloadConfig();
    // 配置文件是否已加载
    bool IsLoaded() const { return this->_isLoaded; };

    // 获取配置值（通用模板方法）
    template <typename T>
    std::optional<T> Get(const std::string &key) const;
    // 获取无符号整型
    std::optional<unsigned int> GetUInt(const std::string &key) const;
    // 获取整型
    std::optional<int> GetInt(const std::string &key) const;
    // 获取字符串
    std::optional<std::string> GetString(const std::string &key) const;
    // 获取布尔值
    std::optional<bool> GetBool(const std::string &key) const;
    // 获取浮点数
    std::optional<double> GetDouble(const std::string &key) const;

private:
    // 实际加载配置文件
    bool LoadConfig();
    // 验证配置路径
    bool ValidateConfigPath(const std::string &configPath = std::string()) const;

    std::shared_ptr<nlohmann::json> _configJson;
    std::string _configFilePath;
    bool _isLoaded;
};

#endif // CONFIGREADER_H
