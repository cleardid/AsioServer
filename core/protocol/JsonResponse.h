#ifndef JSONRESPONSE_H
#define JSONRESPONSE_H

#include <string>
#include <cstdint>
#include "../../infra/util/json.hpp"

// 统一 JSON Response 构造器
class JsonResponse
{
public:
    // 成功响应
    static nlohmann::json Ok(uint16_t serviceId,
                             uint16_t cmdId,
                             uint32_t seq,
                             const nlohmann::json &data = nlohmann::json::object())
    {
        return {
            {"header",
             {
                 {"serviceId", serviceId},
                 {"cmdId", cmdId},
                 {"seq", seq},
             }},
            {"status",
             {
                 {"code", 0},
                 {"message", "OK"},
             }},
            {"data", data}};
    }

    // 错误响应
    static nlohmann::json Error(uint16_t serviceId,
                                uint16_t cmdId,
                                uint32_t seq,
                                int errorCode,
                                const std::string &errorMsg)
    {
        return {
            {"header",
             {
                 {"serviceId", serviceId},
                 {"cmdId", cmdId},
                 {"seq", seq},
             }},
            {"status",
             {
                 {"code", errorCode},
                 {"message", errorMsg},
             }},
            {"data", nullptr}};
    }
};

#endif // JSONRESPONSE_H
