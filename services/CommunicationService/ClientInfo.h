
#ifndef CLIENTINFO_H
#define CLIENTINFO_H

#include <string>

// 客户端信息类
class ClientInfo
{
public:
    // 删除默认构造函数
    ClientInfo() = delete;

    // 显式构造函数
    explicit ClientInfo(std::string ip, std::size_t port, std::string name, bool isLongConn = false)
        : _ip(ip), _port(port), _name(name), _isLongConn(isLongConn)
    {
    }
    ~ClientInfo() = default;

    std::string GetIp() const { return this->_ip; }
    std::size_t GetPort() const { return this->_port; }
    std::string GetName() const { return this->_name; }
    bool IsLongConn() const { return this->_isLongConn; }

private:
    std::string _ip;   // IP地址
    std::size_t _port; // 端口号
    std::string _name; // 用户名 通过消息体赋值
    bool _isLongConn;  // 是否长连接
};

#endif // CLIENTINFO_H