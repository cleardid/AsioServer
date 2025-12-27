
#ifndef MESSAGEHEADER_H
#define MESSAGEHEADER_H

#include <stdint.h>
#include <boost/asio.hpp>

// MessageHeader 定义（带1字节对齐）
#pragma pack(push, 1)
struct MessageHeader
{
    uint16_t magic;     // 魔数 0x55AA
    uint16_t version;   // 协议版本
    uint16_t serviceId; // 服务类型
    uint16_t cmdId;     // 服务内命令
    uint32_t length;    // Body 长度（仅消息体，不含Header）
    uint32_t seq;       // 请求序号

    // 转为网络字节序
    void ToNetwork()
    {
        magic = boost::asio::detail::socket_ops::host_to_network_short(magic);
        version = boost::asio::detail::socket_ops::host_to_network_short(version);
        serviceId = boost::asio::detail::socket_ops::host_to_network_short(serviceId);
        cmdId = boost::asio::detail::socket_ops::host_to_network_short(cmdId);
        length = boost::asio::detail::socket_ops::host_to_network_long(length);
        seq = boost::asio::detail::socket_ops::host_to_network_long(seq);
    }
    // 转为本地字节序
    void ToHost()
    {
        magic = boost::asio::detail::socket_ops::network_to_host_short(magic);
        version = boost::asio::detail::socket_ops::network_to_host_short(version);
        serviceId = boost::asio::detail::socket_ops::network_to_host_short(serviceId);
        cmdId = boost::asio::detail::socket_ops::network_to_host_short(cmdId);
        length = boost::asio::detail::socket_ops::network_to_host_long(length);
        seq = boost::asio::detail::socket_ops::network_to_host_long(seq);
    }
};
#pragma pack(pop)

#endif // MESSAGEHEADER_H