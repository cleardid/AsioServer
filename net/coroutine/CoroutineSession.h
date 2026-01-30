
#ifndef COROUTINESESSION_H
#define COROUTINESESSION_H

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/steady_timer.hpp> // 使用定时器，实现 心跳检测定时器 和 快速心跳处理

#include "../../core/session/CSession.h"

namespace this_coro = boost::asio::this_coro;

class CoroutineSession : public CSession
{
public:
    CoroutineSession(boost::asio::io_context &ioc, CServer *server);
    // 重写父类方法
    void Start() override;

    ~CoroutineSession() override;

private:
    // 启动心跳超时检查
    void StartHeartbeatCheck();

private:
    boost::asio::steady_timer _deadline;                      // 超时定时器
    std::chrono::steady_clock::time_point _last_message_time; // 最后收到消息的时间
};

#endif