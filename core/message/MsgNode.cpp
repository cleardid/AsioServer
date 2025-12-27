
#include "MsgNode.h"

#include <sstream>
#include <iomanip> // 用于 std::hex、std::setw、std::setfill

// 内存管理方法
void MsgNode::Allocate(uint32_t bodyLen)
{
    // 重置消息体长度
    this->_body.resize(bodyLen + 1);
    // 初始化缓冲区（避免内存残留脏数据）
    std::memset(this->_body.data(), 0, bodyLen + 1);
    // 添加字符串结束符，用于调试打印
    this->_body[bodyLen] = '\0';
    // 重置消息头信息
    this->_header.length = bodyLen;
}

//
void MsgNode::Clear()
{
    this->_body.clear();
    // 释放vector多余容量
    this->_body.shrink_to_fit();
    std::memset(&this->_header, 0, sizeof(this->_header));
}

void MsgNode::BuildSendBuffer()
{
    this->_sendBuf.resize(sizeof(MessageHeader) + this->_body.size());
    std::memcpy(this->_sendBuf.data(), &this->_header, sizeof(MessageHeader));
    if (!this->_body.empty())
    {
        std::memcpy(this->_sendBuf.data() + sizeof(MessageHeader),
                    this->_body.data(), this->_body.size());
    }
}

// 输出方法
void MsgNode::Print() const
{
    std::ostringstream oss;
    // 设置格式化：十六进制大写、补零对齐
    oss << std::hex << std::uppercase << std::setfill('0');

    // 逐字段格式化输出（字段名 + 值 + 单位/说明）
    oss << "MessageHeader ["
        << "魔数=0x" << std::setw(4) << static_cast<uint32_t>(this->_header.magic) << ", "
        << "协议版本=" << std::dec << this->_header.version << ", "
        << "服务ID=" << this->_header.serviceId << ", "
        << "命令ID=" << this->_header.cmdId << ", "
        << "消息体长度=" << this->_header.length << "字节, "
        << "请求序号=" << this->_header.seq
        << "]";

    std::cout << oss.str() << std::endl;
    std::cout << this->_body.data() << std::endl;
}