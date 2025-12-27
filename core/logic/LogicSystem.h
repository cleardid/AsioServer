
#ifndef LOGICSYSTEM_H
#define LOGICSYSTEM_H

#include <unordered_map>
#include <queue>
#include <thread>
#include <memory>
#include <string>
#include <functional>
#include <mutex>
#include <iostream>

// 前置声明
class LogicNode;
class CSession;

typedef std::function<void(std::shared_ptr<CSession>, const int16_t &msgId, std::string &&msg)> FunCallBack;

class LogicSystem
{
public:
    // 删除拷贝构造函数
    LogicSystem(const LogicSystem &other) = delete;
    // 删除赋值构造函数
    LogicSystem &operator=(const LogicSystem &other) = delete;

    // 析构函数
    ~LogicSystem();
    // 单例
    static LogicSystem &GetInstance();
    // 对外接口
    void PostMsgToQue(std::shared_ptr<LogicNode>);

private:
    LogicSystem();
    // 处理消息
    void ProcessMsg();
    // 处理消息的辅助方法 成功回调返回 true，否则返回 false
    bool AssistProcessMsg();

    // 逻辑消息队列
    std::queue<std::shared_ptr<LogicNode>> _msgQue;
    // 工作线程
    std::thread _wordThread;
    // 互斥量
    std::mutex _queMutex;
    // 条件变量，用于在队列为空时，将处理线程短暂挂起，直至有新的消息进来，降低 CPU 的轮转
    std::condition_variable _workCond;
    // 是否停止
    bool _isStoped;
};

#endif // LOGICSYSTEM_H