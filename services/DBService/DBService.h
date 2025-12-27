
#ifndef DBSERVICE_H
#define DBSERVICE_H

#include "../IService.h"
#include "DBStruct.h"

class DBService : public IService
{
public:
    DBService();

    uint16_t GetServiceId() const override;

    void RegisterCmd() override;

private:
    // 执行操作
    boost::asio::awaitable<void> OnExecuteCallBack(std::shared_ptr<CSession>, std::shared_ptr<MsgNode>);
    // 关闭连接
    boost::asio::awaitable<void> OnCloseCallBack(std::shared_ptr<CSession>, std::shared_ptr<MsgNode>);

    // 辅助方法 获取响应
    DBRequest ParseRequest(std::shared_ptr<MsgNode>);

    // 辅助方法 执行命令并回传
    boost::asio::awaitable<void> ExecuteCmdAndResponse(std::shared_ptr<CSession>, const MessageHeader &, const DBRequest &);
};

#endif // DBSERVICE_H