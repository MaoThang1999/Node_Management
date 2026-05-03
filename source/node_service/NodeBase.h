#ifndef NODEBASE_H
#define NODEBASE_H

#include "../Utils.h"
#include <thread>
#include <mutex>
#include <atomic>


extern bool g_stateChangeTable[MAX_STATE][MAX_STATE];
extern ThreadSafeQueue<str_MessageState> g_messageNodeQueue;
class NodeState;

class NodeBase{
friend class DeadState;
friend class TimeoutState;
friend class AliveState;
friend class InitState;
public:

    NodeBase();
    ~NodeBase();


    TypeNode getType();
    std::string getNameType();
    StateList getState();
    void setPongState(bool pFlag);
    bool getPongState();
    void changeState(std::unique_ptr<NodeState> p_state);
    void start();
    void stop();
    void resume();
    void setClientInfor(sockaddr_in pClientAddr);
    uint16_t getPortInfo();
    sockaddr_in getClientInfor();
    int getObjectNodeID();

protected:
    TypeNode m_type;
private:
    static int m_nodeID;
    int m_objectNodeID;
    std::unique_ptr<NodeState> m_currentState;
    //int m_nodeMonitorID;
    sockaddr_in m_clientAddr;
    std::atomic<bool> m_running;
    std::atomic<bool> m_pongState;
    std::thread m_thread;
    std::mutex m_mtx;

    std::condition_variable m_cv;
    static void createID();
    void sendPing();
    void handleThread();
};

class NodeState{
public:

    virtual ~NodeState() = default;
    virtual void handleState(NodeBase* node);
     StateList getState(){return m_state;};

protected:
    StateList m_state;
};

class InitState : public NodeState{
public:
    InitState();
    virtual void handleState(NodeBase* node);

};

class AliveState : public NodeState{
public:
    AliveState();
    virtual void handleState(NodeBase* node);

};

class TimeoutState : public NodeState{
public:
    TimeoutState();
    virtual void handleState(NodeBase* node);

};

class DeadState : public NodeState{
public:
    DeadState();
    virtual void handleState(NodeBase* node);

};


#endif /* NODEBASE_H */