#ifndef NODE_MANAGEMENT_H
#define NODE_MANAGEMENT_H

#include <vector>
#include "node_service/NodeBase.h"
#include "node_service/OAMNode.h"
#include "node_service/DataPlaneNode.h"
#include "node_service/ControlPlaneNode.h"
#include "Utils.h"
#include "Packet.h"



class NodeManagament{
public:
    NodeManagament();
    ~NodeManagament();
    static NodeManagament* getInstall();
    // Starts the NodeManagament threads and initializes queues
    bool start();
    // Stops all threads 
    bool stop();
    bool isRunning();

private:
    std::vector<std::unique_ptr<NodeBase>> m_vecNodeBase;
    std::thread m_nodeHandleThread;// Handles outgoing messages (ping/notify)
    std::thread m_IOHandleThread; // Handles incoming messages (register/pong)

    // Thread entry points
    void handleReceivedData();
    void handleSendData();

    // Message handlers
    void checkPingMessage(str_MessageState& pMessageState);
    void checkNotifyMessage(str_MessageState& pMessageState);
    void pushNodeToVector(RigisterPacket* pPacket);
    void checkPongMessage(Packet* pPacket);
    void checkRegisterMessage(RigisterPacket* pPacket);
    std::atomic<bool> m_running;
    std::condition_variable m_cv;
    std::mutex m_mtx;

};

#endif /* NODE_MANAGEMENT_H */