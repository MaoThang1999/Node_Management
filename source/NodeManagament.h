#ifndef NODE_MANAGEMENT_H
#define NODE_MANAGEMENT_H

#include "node_service/NodeBase.h"
#include "node_service/OAMNode.h"
#include "node_service/DataPlaneNode.h"
#include "node_service/ControlPlaneNode.h"
#include "Utils.h"
#include "IOHandler.h"
#include <vector>


class NodeManagament{
public:
    NodeManagament();
    ~NodeManagament();
    static NodeManagament* getInstall();
    void start();
    void stop();
    void resume();

private:
    std::vector<std::unique_ptr<NodeBase>> m_vecNodeBase;
    std::thread m_nodeHandleThread;
    std::thread m_IOHandleThread;
    void handleReceivedData();
    void handleSendData();
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