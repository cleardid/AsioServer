
#ifndef CLIENTMANAGER_H
#define CLIENTMANAGER_H

#include <string>
#include <unordered_map>
#include <mutex>
#include <vector>

// 客户端管理器，用于保存客户端的连接信息
class ClientManager
{
public:
    // 删除拷贝构造函数
    ClientManager(const ClientManager &) = delete;
    // 删除移动构造函数
    ClientManager(ClientManager &&) = delete;
    // 删除赋值运算符
    ClientManager &operator=(const ClientManager &) = delete;
    // 删除移动赋值运算符
    ClientManager &operator=(ClientManager &&) = delete;

    // 单例模式
    static ClientManager &GetInstance();

    // 添加客户端连接信息
    bool AddClient(const std::string &key, const std::string &value);

    // 删除客户端连接信息
    bool RemoveClient(const std::string &key);

    // 获取客户端连接信息
    std::string GetClient(const std::string &key);

    // 获取所有客户端名称
    std::vector<std::string> GetAllClientName();

private:
    ClientManager() = default;
    ~ClientManager() = default;

    // 客户端连接信息 主键为
    std::unordered_map<std::string, std::string> _clientMap;
    // 互斥锁
    std::mutex _mutex;
};

#endif // CLIENTMANAGER_H