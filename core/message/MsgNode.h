
#ifndef MSGNODE_H
#define MSGNODE_H

#include <iostream>
#include <string>
#include <string.h>
#include <vector>

#include "../protocol/MessageHeader.h"

// 消息节点
// 使用 std::vector<char> 管理内存
class MsgNode
{
public:
    // 默认构造函数
    MsgNode() = default;

    // 显式声明构造函数
    explicit MsgNode(uint32_t bodyLen)
    {
        Allocate(bodyLen);
    }

    // 内存管理方法
    void Allocate(uint32_t bodyLen);
    // 释放内存
    void Clear();

    // 头部访问方法

    MessageHeader &GetHeader() { return this->_header; }
    const MessageHeader &GetHeader() const { return this->_header; }
    uint16_t GetServiceId() const { return this->_header.serviceId; }
    uint16_t GetCmdId() const { return this->_header.cmdId; }
    uint32_t GetSeq() const { return this->_header.seq; }

    // 消息体访问方法

    char *GetBody() { return this->_body.data(); }
    const char *GetBody() const { return this->_body.data(); }
    uint32_t GetBodyLen() const { return this->_header.length; }

    // Header buffer

    char *GetHeaderData() { return reinterpret_cast<char *>(&_header); }
    constexpr static std::size_t GetHeaderSize() { return sizeof(MessageHeader); }

    // Send buffer

    void BuildSendBuffer();
    const char *GetSendData() { return this->_sendBuf.data(); }
    std::size_t GetSendSize() const { return this->_sendBuf.size(); }

    // 输出方法
    void Print() const;

protected:
    MessageHeader _header{};
    std::vector<char> _body;

    std::vector<char> _sendBuf; // Header + Body
};

#endif // MSGNODE_H