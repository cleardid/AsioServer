
#include "AsyncSession.h"

#include "../../core/server/CServer.h"
#include "../../core/logic/LogicNode.h"
#include "../../core/logic/LogicSystem.h"

#include "../../infra/log/Logger.h"

AsyncSession::AsyncSession(boost::asio::io_context &ioc, boost::asio::ip::tcp::socket socket, CServer *server)
    : CSession(ioc, std::move(socket), server)
{
}

void AsyncSession::Start()
{
    // 由于 CSession 继承于 std::enable_shared_from_this<CSession>
    // 导致 shared_from_this() 返回的是 CSession 的指针
    // 所以需要将 CSession 的指针转换为 AsyncSession 的指针

    // 在调用 async_read 前进行转换
    auto self = std::static_pointer_cast<AsyncSession>(shared_from_this());

    boost::asio::async_read(this->_socket,
                            boost::asio::buffer(this->_recvNode->GetHeaderData(), MsgNode::GetHeaderSize()),
                            std::bind(&AsyncSession::HandleHeadRead,
                                      self, // 使用已转换的派生类指针
                                      std::placeholders::_1,
                                      std::placeholders::_2));
}

void AsyncSession::HandleHeadRead(const boost::system::error_code &error, std::size_t bytes_transferred)
{
    if (!error)
    {
        if (bytes_transferred == 0)
        {
            LOG_ERROR << "AsyncSession: Client Close a Connect." << std::endl;
            Close();
            this->_server->DelSessionByUuid(this->_uuid);
        }

        if (bytes_transferred < MsgNode::GetHeaderSize())
        {
            LOG_ERROR << "AsyncSession: Received Head Error : Length Too Low" << std::endl;
            Close();
            this->_server->DelSessionByUuid(this->_uuid);
        }

        // 转为本地主机字节序 必须转换！！！
        this->_recvNode->GetHeader().ToHost();
        // 为消息节点分配内存
        this->_recvNode->Allocate(this->_recvNode->GetHeader().length);

        // 在调用 async_read 前进行转换
        auto self = std::static_pointer_cast<AsyncSession>(shared_from_this());
        // 开始读取消息体内容
        boost::asio::async_read(this->_socket,
                                boost::asio::buffer(this->_recvNode->GetBody(), this->_recvNode->GetBodyLen()),
                                std::bind(&AsyncSession::HandleMsgRead,
                                          self,
                                          std::placeholders::_1,
                                          std::placeholders::_2));
    }
    else
    {
        LOG_ERROR << "AsyncSession: Received Head Error occurred: " << error.message() << std::endl;
        Close();
        this->_server->DelSessionByUuid(this->_uuid);
    }
}

void AsyncSession::HandleMsgRead(const boost::system::error_code &error, std::size_t bytes_transferred)
{
    if (!error)
    {
        if (bytes_transferred < this->_recvNode->GetBodyLen())
        {
            LOG_ERROR << "AsyncSession: Received Msg Error : Length Too Low" << std::endl;
            Close();
            this->_server->DelSessionByUuid(this->_uuid);
        }

        // 打印消息内容
        this->_recvNode->Print();

        LOG_INFO << "CoroutineSession: Recived Node to LogicSystem... " << std::endl;
        auto msg = std::move(_recvNode);
        this->_recvNode = std::make_shared<MsgNode>();

        LogicSystem::GetInstance().PostMsgToQue(std::make_shared<LogicNode>(shared_from_this(), msg));

        // 继续读取头部信息

        // 清空消息节点
        this->_recvNode->Clear();
        // 在调用 async_read 前进行转换
        auto self = std::static_pointer_cast<AsyncSession>(shared_from_this());

        boost::asio::async_read(this->_socket,
                                boost::asio::buffer(this->_recvNode->GetHeaderData(), MsgNode::GetHeaderSize()),
                                std::bind(&AsyncSession::HandleHeadRead,
                                          self, // 使用已转换的派生类指针
                                          std::placeholders::_1,
                                          std::placeholders::_2));
    }
    else
    {
        LOG_ERROR << "AsyncSession: Received Msg Error occurred: " << error.message() << std::endl;
        Close();
        this->_server->DelSessionByUuid(this->_uuid);
    }
}
