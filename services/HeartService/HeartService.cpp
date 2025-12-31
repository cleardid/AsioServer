
#include "HeartService.h"
#include "../CommunicationService/ClientInfo.h"
#include "../CommunicationService/ClientManager.h"

#include "../../infra/log/Logger.h"
#include "../../core/protocol/JsonResponse.h"

HeartService::HeartService()
{
}

uint16_t HeartService::GetServiceId() const
{
    return SERVICE_HEART;
}

void HeartService::RegisterCmd()
{
    this->_cmdMap[HEART_RECV] = std::bind(&HeartService::OnRecvHeartCallBack, this, std::placeholders::_1, std::placeholders::_2);
}

boost::asio::awaitable<void> HeartService::OnRecvHeartCallBack(std::shared_ptr<CSession> session, std::shared_ptr<MsgNode> msg)
{
    LOG_INFO << "CommunicationService OnRecvHeartCallBack" << "\n";

    // 获取请求头
    auto &hdr = msg->GetHeader();

    // 心跳回应命令
    hdr.cmdId = HEART_ACK;

    try
    {
        // 获取客户端的名称
        auto client = session->GetClientInfo();
        // 如果不存在客户端的信息
        if (client == nullptr)
        {
            // 声明错误信息
            std::string errorMsg = "client does not exists";

            auto resp = JsonResponse::Error(
                hdr.serviceId, hdr.cmdId, hdr.seq,
                00001, errorMsg);

            // 回传结果
            session->Send(hdr, resp);
        }
        else
        {
            // 获取当前时间
            auto now = std::chrono::system_clock::now();
            // 获取时间间隔 s
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - client->GetLastConnectTime());

            // TODO: 超时机制

            // 更新连接时间
            client->SetLastConnectTime(std::chrono::system_clock::now());

            // 组装 JSON 信息
            nlohmann::json data = {
                {"time", duration.count()}};

            auto resp = JsonResponse::Ok(
                hdr.serviceId, hdr.cmdId, hdr.seq,
                {{"result", data}});

            // 回传结果
            session->Send(hdr, resp);
        }
    }
    catch (const std::exception &e)
    {
        // 声明错误信息
        std::string errorMsg = "UNKNOWN ERROR";

        auto resp = JsonResponse::Error(
            hdr.serviceId, hdr.cmdId, hdr.seq,
            9999, errorMsg);

        // 回传结果
        session->Send(hdr, resp);
    }

    co_return;
}
