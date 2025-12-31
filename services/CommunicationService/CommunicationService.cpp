
#include "CommunicationService.h"
#include "ClientInfo.h"
#include "ClientManager.h"

#include "../../infra/log/Logger.h"
#include "../../core/protocol/JsonResponse.h"

#include <memory>

CommunicationService::CommunicationService()
{
}

uint16_t CommunicationService::GetServiceId() const
{
    return SERVICE_COMMUNICATION;
}

void CommunicationService::RegisterCmd()
{
    this->_cmdMap[COMMUINICATION_REGISTER] = std::bind(&CommunicationService::OnCreateCallBack, this, std::placeholders::_1, std::placeholders::_2);
    this->_cmdMap[COMMUINICATION_CLOSE] = std::bind(&CommunicationService::OnCloseCallBack, this, std::placeholders::_1, std::placeholders::_2);
    this->_cmdMap[COMMUINICATION_SEND] = std::bind(&CommunicationService::OnSendCallBack, this, std::placeholders::_1, std::placeholders::_2);
    this->_cmdMap[COMMUINICATION_SHOW] = std::bind(&CommunicationService::OnShowCallBack, this, std::placeholders::_1, std::placeholders::_2);
}

boost::asio::awaitable<void> CommunicationService::OnCreateCallBack(std::shared_ptr<CSession> session, std::shared_ptr<MsgNode> msg)
{
    LOG_INFO << "CommunicationService OnCreateCallBack" << "\n";

    // 获取请求头
    auto &hdr = msg->GetHeader();

    try
    {
        // 获取 socket
        auto &socket = session->GetSocket();
        // 获取 endpoint
        auto endpoint = socket.remote_endpoint();
        // 获取 ip 地址
        auto ip = endpoint.address().to_string();
        // 获取端口号
        auto port = endpoint.port();

        auto reqJson = nlohmann::json::parse(
            std::string(msg->GetBody(), msg->GetBodyLen()));

        // 获取 target 信息 其包含了客户端的信息
        auto &target = reqJson.at("target");
        // 获取客户端指定的名称
        auto name = target.at("name").get<std::string>();

        // 声明 ClientInfo
        auto clientInfo = std::make_shared<ClientInfo>(ip, port, name);

        // 将 clientInfo 保存到 session 中
        session->SetClientInfo(clientInfo);

        // 添加至客户端管理器
        auto success = ClientManager::GetInstance().AddClient(name, session->GetUuid());

        nlohmann::json resp;
        if (success)
        {
            resp = JsonResponse::Ok(
                hdr.serviceId, hdr.cmdId, hdr.seq);
        }
        else
        {
            // 声明错误信息
            std::string errorMsg = "client name already exists";

            resp = JsonResponse::Error(
                hdr.serviceId, hdr.cmdId, hdr.seq,
                20001, errorMsg);
        }

        // 回传结果
        session->Send(hdr, resp);
    }
    catch (const std::exception &e)
    {
        // 声明错误信息
        std::string errorMsg = "invalid request json";

        auto resp = JsonResponse::Error(
            hdr.serviceId, hdr.cmdId, hdr.seq,
            29999, errorMsg);

        // 回传结果
        session->Send(hdr, resp);

        LOG_ERROR << e.what() << '\n';
    }

    co_return;
}

boost::asio::awaitable<void> CommunicationService::OnCloseCallBack(std::shared_ptr<CSession> session, std::shared_ptr<MsgNode> msg)
{
    LOG_INFO << "CommunicationService OnCloseCallBack" << "\n";

    // 获取请求头
    auto &hdr = msg->GetHeader();

    try
    {
        // 获取 target 信息 其包含了客户端的信息
        auto reqJson = nlohmann::json::parse(
            std::string(msg->GetBody(), msg->GetBodyLen()));

        auto &target = reqJson.at("target");
        // 获取客户端指定的名称
        auto name = target.at("name").get<std::string>();

        // 客户端主动关闭连接
        session->ClientClose();

        // 从客户端管理器中移除
        auto success = ClientManager::GetInstance().RemoveClient(name);

        nlohmann::json resp;
        if (success)
        {
            resp = JsonResponse::Ok(
                hdr.serviceId, hdr.cmdId, hdr.seq);
        }
        else
        {
            // 声明错误信息
            std::string errorMsg = "client name not exists";

            resp = JsonResponse::Error(
                hdr.serviceId, hdr.cmdId, hdr.seq,
                20002, errorMsg);
        }

        // 回传结果
        session->Send(hdr, resp);
    }
    catch (const std::exception &e)
    {
        // 声明错误信息
        std::string errorMsg = "invalid request json";

        auto resp = JsonResponse::Error(
            hdr.serviceId, hdr.cmdId, hdr.seq,
            29999, errorMsg);

        // 回传结果
        session->Send(hdr, resp);

        LOG_ERROR << e.what() << '\n';
    }

    co_return;
}

boost::asio::awaitable<void> CommunicationService::OnSendCallBack(std::shared_ptr<CSession> session, std::shared_ptr<MsgNode> msg)
{
    LOG_INFO << "CommunicationService OnSendCallBack" << "\n";
    // 获取请求头
    auto &hdr = msg->GetHeader();

    try
    {
        // 获取 target 信息 其包含了客户端的信息
        auto reqJson = nlohmann::json::parse(
            std::string(msg->GetBody(), msg->GetBodyLen()));

        auto &target = reqJson.at("target");
        // 获取发送方的名称
        auto name = target.at("name").get<std::string>();
        // 获取客户端指定的客户端
        auto clientName = target.at("client").get<std::string>();
        // 获取消息内容
        auto message = target.at("message").get<std::string>();

        // 获取接收端的 session 的 uuid
        auto clientSessionUuid = ClientManager::GetInstance().GetClient(clientName);
        // 未找到接收端
        if (clientSessionUuid.empty())
        {
            // 声明错误信息
            std::string errorMsg = "client name not exists";

            auto resp = JsonResponse::Error(
                hdr.serviceId, hdr.cmdId, hdr.seq,
                20002, errorMsg);

            // 回传结果
            session->Send(hdr, resp);
        }

        // 组装 JSON 信息
        nlohmann::json mJson = {
            {"from", name},
            {"message", message}};

        // 发送信息
        auto success = session->SendToOtherSession(clientSessionUuid, hdr, message);

        if (success)
        {
            auto resp = JsonResponse::Ok(
                hdr.serviceId, hdr.cmdId, hdr.seq);

            // 回传结果
            session->Send(hdr, resp);
        }
        else
        {
            // 声明错误信息
            std::string errorMsg = "client name or session not exists";

            auto resp = JsonResponse::Error(
                hdr.serviceId, hdr.cmdId, hdr.seq,
                20003, errorMsg);

            // 回传结果
            session->Send(hdr, resp);
        }
    }
    catch (const std::exception &e)
    {
        // 声明错误信息
        std::string errorMsg = "invalid request json";

        auto resp = JsonResponse::Error(
            hdr.serviceId, hdr.cmdId, hdr.seq,
            29999, errorMsg);

        // 回传结果
        session->Send(hdr, resp);

        LOG_ERROR << e.what() << '\n';
    }

    co_return;
}

boost::asio::awaitable<void> CommunicationService::OnShowCallBack(std::shared_ptr<CSession> session, std::shared_ptr<MsgNode> msg)
{
    LOG_INFO << "CommunicationService OnShowCallBack" << "\n";

    // 获取请求头
    auto &hdr = msg->GetHeader();

    try
    {
        // 获取所有客户端的名称
        auto clients = ClientManager::GetInstance().GetAllClientName();

        // 组装 JSON 信息
        nlohmann::json data;

        data["clients"] = clients;
        data["count"] = clients.size();

        auto resp = JsonResponse::Ok(
            hdr.serviceId, hdr.cmdId, hdr.seq,
            {{"result", data}});

        // 回传结果
        session->Send(hdr, resp);
    }
    catch (const std::exception &e)
    {
        // 声明错误信息
        std::string errorMsg = "UNKNOWN ERROR";

        auto resp = JsonResponse::Error(
            hdr.serviceId, hdr.cmdId, hdr.seq,
            29999, errorMsg);

        // 回传结果
        session->Send(hdr, resp);
    }

    co_return;
}
