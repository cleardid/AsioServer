#include "ClientManager.h"

ClientManager &ClientManager::GetInstance()
{
    static ClientManager instance;
    return instance;
}

bool ClientManager::AddClient(const std::string &key, const std::string &value)
{
    // 加锁
    std::lock_guard<std::mutex> lock(this->_mutex);

    // 不存在则添加，存在则不添加
    if (this->_clientMap.find(key) == this->_clientMap.end())
    {
        this->_clientMap[key] = value;
        return true;
    }

    // 默认返回 false
    return false;
}

bool ClientManager::RemoveClient(const std::string &key)
{
    // 加锁
    std::lock_guard<std::mutex> lock(this->_mutex);

    if (this->_clientMap.find(key) != this->_clientMap.end())
    {
        this->_clientMap.erase(key);
        return true;
    }

    // 默认返回 false
    return false;
}

std::string ClientManager::GetClient(const std::string &key)
{
    // 加锁
    std::lock_guard<std::mutex> lock(this->_mutex);

    if (this->_clientMap.find(key) != this->_clientMap.end())
    {
        return this->_clientMap.at(key);
    }

    // 默认返回空字符串
    return std::string();
}

std::vector<std::string> ClientManager::GetAllClientName()
{
    // 加锁
    std::lock_guard<std::mutex> lock(this->_mutex);

    std::vector<std::string> names;
    for (auto &item : this->_clientMap)
    {
        names.push_back(item.first);
    }

    return names;
}
