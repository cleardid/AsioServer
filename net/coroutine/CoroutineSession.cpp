
#include "CoroutineSession.h"

#include "../../core/server/CServer.h"
#include "../../core/logic/LogicNode.h"
#include "../../core/logic/LogicSystem.h"

#include "../../infra/log/Logger.h"
#include "../../infra/util/StringFormat.h"

CoroutineSession::CoroutineSession(boost::asio::io_context &ioc, CServer *server)
    : CSession(ioc, server)
{
}

void CoroutineSession::Start()
{
    // 开启协程
    boost::asio::co_spawn(this->_ioc, [this]() -> boost::asio::awaitable<void>
                          {
                 try
                 {
                    for (; !this->_bStop; )
                    {
                        // 重置消息节点
                        this->_recvNode->Clear();
                        // 开始读取头部信息
                        std::size_t len = co_await boost::asio::async_read(this->_socket, boost::asio::buffer(this->_recvNode->GetHeaderData(), MsgNode::GetHeaderSize()), boost::asio::use_awaitable);
                        if (len == 0)
                        {
                            LOG_ERROR << "CoroutineSession: Client Close a Connect." << std::endl;
                            Close();
                            this->_server->DelSessionByUuid(this->_uuid);
                            co_return;
                        }

                        // 转为本地主机字节序 必须转换！！！
                        this->_recvNode->GetHeader().ToHost();

                        // 为消息节点分配内存
                        this->_recvNode->Allocate(this->_recvNode->GetHeader().length);

                        // 开始读取消息信息
                        len = co_await boost::asio::async_read(this->_socket, boost::asio::buffer(this->_recvNode->GetBody(), this->_recvNode->GetBodyLen()), boost::asio::use_awaitable);
                        if (len == 0)
                        {
                            LOG_ERROR << "CoroutineSession: Client Close a Connect." << std::endl;
                            Close();
                            this->_server->DelSessionByUuid(this->_uuid);
                            co_return;
                        }
                        // 输出信息
                        this->_recvNode->Print();
                        LOG_INFO << "CoroutineSession: Recived Node to LogicSystem... " << std::endl;
                        auto msg = std::move(_recvNode);
                        this->_recvNode = std::make_shared<MsgNode>();

                        // TODO: 将消息节点传递给逻辑处理系统
                        LogicSystem::GetInstance().PostMsgToQue(std::make_shared<LogicNode>(shared_from_this(), msg));

                    }
                 }
                 catch (boost::system::system_error& e)
                 {
                    if (e.code().value() == 10054) {
                        LOG_INFO << "客户端正常断开连接" << '\n';
                    } 
                    else 
                    {
                        LOG_ERROR << "网络错误 [" << e.code().value() << "]: " 
                                << ConvertStringToUTF8(e.code().message()) << '\n';
                    }
                     Close();
                     this->_server->DelSessionByUuid(this->_uuid);
                 } }, boost::asio::detached);
}
