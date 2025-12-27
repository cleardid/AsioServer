
#ifndef ASYNCSESSION_H
#define ASYNCSESSION_H

#include "../../core/session/CSession.h"

#include <boost/asio.hpp>

class AsyncSession : public CSession
{
public:
    AsyncSession(boost::asio::io_context &ioc, CServer *server);
    // 重写父类方法
    void Start() override;

private:
    // 简易读取数据的方法 用于异步服务器实现
    void HandleHeadRead(const boost::system::error_code &error, std::size_t bytes_transferred);
    void HandleMsgRead(const boost::system::error_code &error, std::size_t bytes_transferred);
};

#endif // ASYNCSESSION_H