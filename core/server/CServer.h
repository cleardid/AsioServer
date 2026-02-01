
#ifndef CSERVER_H
#define CSERVER_H

#include <iostream>
#include <string>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <boost/asio.hpp>

#include "../common/Const.h"

// 前置声明
class CSession;

// 服务类
class CServer
{
public:
    // 构造函数
    CServer(boost::asio::io_context &ioc, const uint16_t port, ASIO_TYPE type = ASIO_TYPE::COROUTINE);
    // 析构函数
    ~CServer();

    // 对外接口
    // 清空所有会话
    void ClearSession();
    // 删除 uuid 对应的 Session
    void DelSessionByUuid(const std::string &);
    // 获取 uuid 对应的 Session
    std::shared_ptr<CSession> GetSessionByUuid(const std::string &);

private:
    // 开始监听
    // 协程方式
    void StartCoroutineAccpet();
    // 异步方式
    void StartAsyncAccpet();
    // 处理接收操作
    // void HandleAccpet(std::shared_ptr<CSession>, const boost::system::error_code &error);
    // 辅助启动方法
    // void AssistStart(bool isCreateFunc = false);

    // 私有属性
    // 上下文
    boost::asio::io_context &_ioContext;
    // 端口号
    uint16_t _port;
    // tcp服务器监听器
    boost::asio::ip::tcp::acceptor _acceptor;
    // 会话管理字典
    std::unordered_map<std::string, std::shared_ptr<CSession>> _sessionMap;
    // 信号量
    std::mutex _mutex;
    // 服务类型
    ASIO_TYPE _type;

    // 是否为第一次创建
    bool _isFirstCreate = true;
};

#endif // CSERVER_H