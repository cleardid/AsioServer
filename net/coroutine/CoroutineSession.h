
#ifndef COROUTINESESSION_H
#define COROUTINESESSION_H

#include <boost/asio/co_spawn.hpp>
#include <boost/asio/detached.hpp>

#include "../../core/session/CSession.h"

namespace this_coro = boost::asio::this_coro;

class CoroutineSession : public CSession
{
public:
    CoroutineSession(boost::asio::io_context &ioc, CServer *server);
    // 重写父类方法
    void Start() override;
};

#endif