
#ifndef CSESSION_H
#define CSESSION_H

#include <boost/asio.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>

#include <queue>
#include <memory>
#include <mutex>

#include "../../infra/util/json.hpp"
#include "../message/MsgNode.h"

// 前置声明
class CServer;
class ClientInfo;

class CSession : public std::enable_shared_from_this<CSession>
{
public:
    CSession(boost::asio::io_context &ioc, CServer *server);
    // 虚析构函数
    virtual ~CSession();

    // 对外接口
    // 获取 socket
    boost::asio::ip::tcp::socket &GetSocket() { return this->_socket; }

    // 纯虚方法 子类实现
    virtual void Start() = 0;

    void Close();
    // 对外接口
    void Send(const MessageHeader &header, const std::string &);
    void Send(const MessageHeader &header, const nlohmann::json &body);
    // 获取唯一标识符
    const std::string &GetUuid() const { return this->_uuid; };
    // 获取 IO 上下文
    boost::asio::io_context &GetIoContext() { return this->_ioc; }

    // 设置客户端信息
    void SetClientInfo(std::shared_ptr<ClientInfo> clientInfo) { this->_clientInfo = std::move(clientInfo); }
    // 获取客户端信息
    std::shared_ptr<ClientInfo> &GetClientInfo() { return this->_clientInfo; }
    // 客户端主动关闭会话
    void ClientClose();
    // 通过 server 访问其他会话
    bool SendToOtherSession(const std::string &uuid, const MessageHeader &header, const std::string &body);

protected:
    // 处理写事件
    void HandleWrite(const boost::system::error_code &error);
    void DoSend(std::shared_ptr<MsgNode>);
    void DoWrite();

    // 私有发送事件
    void Send(const MessageHeader &header, const char *body, const uint32_t &len);

    // 此会话的唯一标识
    std::string _uuid;
    // 由哪个上下文管理
    boost::asio::io_context &_ioc;
    // 由哪个服务管理
    CServer *_server;
    // 处理连接信息
    boost::asio::ip::tcp::socket _socket;
    // 互斥量 用于发送队列的多线程保护
    std::mutex _mutex;
    // 头部信息
    std::shared_ptr<MsgNode> _recvNode;
    // 发送队列
    std::queue<std::shared_ptr<MsgNode>> _sendQue;
    // 原子类型标志变量，确保线程安全，表示此会话关闭
    std::atomic<bool> _bStop;
    // 客户端信息，默认不创建，仅在通信服务中创建
    std::shared_ptr<ClientInfo> _clientInfo;
};

#endif // CSESSION_H