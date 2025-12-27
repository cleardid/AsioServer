
#ifndef LOGICNODE_H
#define LOGICNODE_H

#include <memory>

// 前置声明
class CSession;
class MsgNode;

// 用于传递至 LogicSystem
class LogicNode
{
public:
    LogicNode(std::shared_ptr<CSession> session,
              std::shared_ptr<MsgNode> msg)
        : _session(session),
          _msg(msg)
    {
    }

    std::shared_ptr<CSession> GetSession() { return this->_session; }
    std::shared_ptr<MsgNode> GetRecvNode() { return this->_msg; }

private:
    std::shared_ptr<CSession> _session;
    std::shared_ptr<MsgNode> _msg;
};

#endif // LOGICNODE_H