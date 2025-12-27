
#include "DBService.h"
#include "DBExecutor.h"
#include <iostream>

#include "../../infra/log/Logger.h"
#include "../../core/protocol/JsonResponse.h"

DBService::DBService()
{
    // // 注册消息
    // RegisterCmd();
}

uint16_t DBService::GetServiceId() const
{
    return SERVICE_DB;
}

void DBService::RegisterCmd()
{
    this->_cmdMap[DB_EXECUTE] = std::bind(&DBService::OnExecuteCallBack, this,
                                          std::placeholders::_1, std::placeholders::_2);
    this->_cmdMap[DB_CLOSE] = std::bind(&DBService::OnCloseCallBack, this,
                                        std::placeholders::_1, std::placeholders::_2);
}

boost::asio::awaitable<void> DBService::OnExecuteCallBack(std::shared_ptr<CSession> session, std::shared_ptr<MsgNode> msg)
{
    LOG_DEBUG << "DBService: OnExecuteCallBack called." << std::endl;
    // 获取请求头
    auto &hdr = msg->GetHeader();
    auto req = ParseRequest(msg);

    // 执行并回传
    co_await ExecuteCmdAndResponse(session, hdr, req);

    co_return;
}

boost::asio::awaitable<void> DBService::OnCloseCallBack(std::shared_ptr<CSession> session, std::shared_ptr<MsgNode> msg)
{
    // 获取请求头
    auto &hdr = msg->GetHeader();
    auto req = ParseRequest(msg);

    // 设置命令为关闭连接
    req.cmd = "close";
    // 执行并回传
    co_await ExecuteCmdAndResponse(session, hdr, req);

    co_return;
}

DBRequest DBService::ParseRequest(std::shared_ptr<MsgNode> msg)
{
    auto reqJson = nlohmann::json::parse(
        std::string(msg->GetBody(), msg->GetBodyLen()));

    DBRequest req;

    // 获取 target 信息 其包含了数据库的连接信息
    auto &target = reqJson.at("target");
    // 获取数据库类型
    auto dbType = target.at("type").get<std::string>();

    // 根据数据库类型进行处理
    if (dbType == "mysql")
    {
        // 获取连接信息
        auto &connInfo = target.at("connInfo");
        // 获取自定义的连接池标识
        req.key.type = "mysql";
        req.key.ident = connInfo.at("host").get<std::string>() + ":" +
                        std::to_string(connInfo.at("port").get<int>()) + "/" +
                        connInfo.at("database").get<std::string>();
    }

    // 获取 action 信息
    auto &action = reqJson.at("action");
    // 获取 sql 语句
    req.sql = action.at("sql").get<std::string>();

    return req;
}

boost::asio::awaitable<void> DBService::ExecuteCmdAndResponse(std::shared_ptr<CSession> session, const MessageHeader &hdr, const DBRequest &req)
{
    auto result = co_await DBExecutor::GetInstance().ExecuteRequest(req);

    nlohmann::json resp;
    if (result.success)
    {
        resp = JsonResponse::Ok(
            hdr.serviceId, hdr.cmdId, hdr.seq,
            {{"result", result.MakeResultJson()}});
    }
    else
    {
        resp = JsonResponse::Error(
            hdr.serviceId, hdr.cmdId, hdr.seq,
            10001, result.errorMsg);
    }

    // 回传结果
    session->Send(hdr, resp);
    co_return;
}
