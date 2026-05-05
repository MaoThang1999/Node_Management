/**
 * @file NodeManagament.cpp
 * @brief Implements the NodeManagament singleton – monitors nodes via register/ping/pong.
 */
#include "NodeManagament.h"


ThreadSafeQueue<str_MessageState> g_messageNodeQueue;
ThreadSafeQueue<Packet*> g_sendIOQueue;
ThreadSafeQueue<Packet*> g_receivedIOQueue;

// row: previous State; col: current State
bool g_stateChangeTable[MAX_STATE][MAX_STATE] = {
                                                    {0, 1, 1, 0}, 
                                                    {0, 0, 1, 0}, 
                                                    {0, 1, 0, 1}, 
                                                    {1, 0, 0 ,0}    };


NodeManagament::NodeManagament(){
    m_running.store(false,std::memory_order_release);
}
NodeManagament::~NodeManagament(){
    stop();
    
}

bool NodeManagament::start(){
    if(!(m_running.load(std::memory_order_acquire))){
        m_running.store(true,std::memory_order_release);

        g_receivedIOQueue.start();
        g_messageNodeQueue.start();

        m_IOHandleThread = std::thread([&](){handleReceivedData();});
        m_nodeHandleThread = std::thread([&](){handleSendData();});
        return true;
    }
    return false;
}

bool NodeManagament::stop(){

    if(m_running.load(std::memory_order_acquire)){
        m_running.store(false,std::memory_order_release);
        g_receivedIOQueue.stop();
        g_messageNodeQueue.stop();

        if(m_IOHandleThread.joinable()){
            m_IOHandleThread.join();
        }

        if(m_nodeHandleThread.joinable()){
            m_nodeHandleThread.join();
        }

        return true;
    }else{
        return false;
        // do nothing
    }

}

bool NodeManagament::isRunning(){
    if(m_running.load(std::memory_order_acquire)){
        return true;
    }else{
        return false;
    }
}

NodeManagament* NodeManagament::getInstall(){
        static NodeManagament* m_install = nullptr;
        if(m_install == nullptr){
            m_install = new NodeManagament();
        }
        
        return m_install;
}

// -----------------------------------------------------------------------------
// Thread: handles incoming packets from g_receivedIOQueue.
// Processes REGISTER and PONG messages.
// -----------------------------------------------------------------------------
void NodeManagament::handleReceivedData(){

    Packet* packet = new Packet();
    while(true){

        g_receivedIOQueue.waitAndPop(packet);
        // Check stop condition after waking up
        if(!m_running.load(std::memory_order_acquire)){
            break;
        }

        if(packet != nullptr)
        {
            
            if(packet->getMessageType() == TypeMessage::PONG){
                
                checkPongMessage(packet);

            }else if(packet->getMessageType() == TypeMessage::REGISTER ){ //0: register message
                // cast to register object 
                RigisterPacket* rPacket =  static_cast<RigisterPacket*>(packet);
                checkRegisterMessage(rPacket);
                
            }else{
                // do nothing
            }
            delete packet;
            packet = nullptr;
        }
    }
    if(packet != nullptr){
        delete packet;
        packet = nullptr;
    }
}

// -----------------------------------------------------------------------------
// Thread: handles outgoing messages from g_messageNodeQueue.
// Processes PING (type 0) and NOTIFY (type 1) messages.
// -----------------------------------------------------------------------------
void NodeManagament::handleSendData(){
    
    str_MessageState messageNode;
    while(true){
        g_messageNodeQueue.waitAndPop(messageNode);
        if(!m_running.load(std::memory_order_acquire)){
            break;
        }
        
        // If this is ping message
        if(messageNode.m_messageType == 0){

            checkPingMessage(messageNode);

        // If this is state change message
        }else if(messageNode.m_messageType == 1){
             
            checkNotifyMessage(messageNode);
        }else{
            // do nothing
        }

    }
}

// -----------------------------------------------------------------------------
// Updates the corresponding node's PongState when a PONG is received.
// -----------------------------------------------------------------------------
void NodeManagament::checkPongMessage(Packet* pPacket){

    if(pPacket == nullptr){
        return;
    }
    if(!m_vecNodeBase.empty()){

        for(auto it = m_vecNodeBase.begin(); it != m_vecNodeBase.end(); ++it){
            sockaddr_in clientAddr = pPacket->getSockaddr();
            if((*it)->getPortInfo() == clientAddr.sin_port){

                (*it)->setPongState(true);
                break;
            }
        }

    }else{
        // do nothing
    }
}

// -----------------------------------------------------------------------------
// Handles REGISTER message: adds a new node if not already present.
// -----------------------------------------------------------------------------
void NodeManagament::checkRegisterMessage(RigisterPacket* pPacket){

    if(pPacket == nullptr){
        return;
    }
    if(!m_vecNodeBase.empty()){

        auto it = m_vecNodeBase.begin();
        for(; it != m_vecNodeBase.end(); ++it){
            sockaddr_in clientAddr = pPacket->getSockaddr();
            if((*it)->getPortInfo() == clientAddr.sin_port){
                break;
            }
        }

        if(it == m_vecNodeBase.end()){

            pushNodeToVector(pPacket);

        }else{
            // This node is monitored
        }

    }else{

        pushNodeToVector(pPacket);
    }
}

// -----------------------------------------------------------------------------
// Creates a new node object based on the packet's node type and adds it to the vector.
// -----------------------------------------------------------------------------
void NodeManagament::pushNodeToVector(RigisterPacket* pPacket){
    if(pPacket == nullptr){
        return;
    }
    int iDNode = 0;
    int numNode = m_vecNodeBase.size();
    uint16_t port = ntohs(pPacket->getSockaddr().sin_port);
    switch(pPacket->getTypeNode()){
        
        case TypeNode::DATA_PLANE:
        {
            std::unique_ptr<DataPlaneNode> dataPlaneObj = std::make_unique<DataPlaneNode>();
            dataPlaneObj->setClientInfor(pPacket->getSockaddr());
            iDNode = dataPlaneObj->getObjectNodeID();
            m_vecNodeBase.push_back(std::move(dataPlaneObj));
            m_vecNodeBase.back()->start();
            DEBUG_LOG("Add a DATA_PLANENode to NoM, NumofNode[" << numNode + 1 << "] NodeID["  << iDNode << "] Port[" << port << "]");
            break;
        }
        case TypeNode::CONTROL_PLANE:
        {

            std::unique_ptr<ControlPlaneNode> controlPlaneObj = std::make_unique<ControlPlaneNode>();
            sockaddr_in clientInfo = pPacket->getSockaddr();
            controlPlaneObj->setClientInfor(clientInfo);
            iDNode = controlPlaneObj->getObjectNodeID();
            m_vecNodeBase.push_back(std::move(controlPlaneObj));
            m_vecNodeBase.back()->start();
            DEBUG_LOG("Add a CONTROL_PLANE to NoM, NumofNode[" << numNode + 1<< "] NodeID["  << iDNode << "] Port[" << port << "]");
            break;
        }
        case TypeNode::OAM:
        {
            std::unique_ptr<OAMNode> oamObj = std::make_unique<OAMNode>();
            oamObj->setClientInfor(pPacket->getSockaddr());
            iDNode = oamObj->getObjectNodeID();
            m_vecNodeBase.push_back(std::move(oamObj));
            m_vecNodeBase.back()->start();
            DEBUG_LOG("Add an OAM to NoM, NumofNode[" << numNode + 1<< "] NodeID["  << iDNode << "] Port[" << port << "]");
            break;
        }   
        default:
            break;
    }
}

// -----------------------------------------------------------------------------
// Creates a PING packet and pushes it to the send queue.
// -----------------------------------------------------------------------------
void NodeManagament::checkPingMessage(str_MessageState& pMessageState){

    PingPacket* pingPacket = new PingPacket();
    pingPacket->setMessageType(TypeMessage::PING);
    pingPacket->setMessage(pMessageState.m_message);
    pingPacket->setPayloadLengh(PING_PLAYLOADLENGTH);
    pingPacket->setSockaddr(pMessageState.m_clientAddr);
    g_sendIOQueue.push(pingPacket, true);

}

// -----------------------------------------------------------------------------
// Handles NOTIFY messages.
// - If state == TIMEOUT: sends a NOTIFY packet to all monitored nodes.
// - If state == DEAD: erases the dead node from the vector.
// -----------------------------------------------------------------------------
void NodeManagament::checkNotifyMessage(str_MessageState& pMessageState){

    if(pMessageState.m_state == StateList::TIMEOUT){

        for(auto it = m_vecNodeBase.begin(); it != m_vecNodeBase.end(); ++it){
            // Send to all node 
            NotifyPacket* notifyPacket = new NotifyPacket();

            // Info of the received node
            notifyPacket->setSockaddr((*it)->getClientInfor());

            notifyPacket->setMessageType(TypeMessage::NOTIFY);
            notifyPacket->setMessage(pMessageState.m_message);
            notifyPacket->setPayloadLengh(NOTIFY_PLAYLOADLENGTH);
            notifyPacket->setTypeNode(pMessageState.m_typeNode);
            notifyPacket->setStateNodeNotify(pMessageState.m_state);
            notifyPacket->setPortNodeotify(pMessageState.m_clientAddr.sin_port);
            g_sendIOQueue.push(notifyPacket, true);
        }
    }
    else if(pMessageState.m_state == StateList::DEAD){
        // Delete the dead node.
        if(pMessageState.m_state == StateList::DEAD){
            for(auto it = m_vecNodeBase.begin(); it != m_vecNodeBase.end(); ++it){
                uint16_t portInfo = (*it)->getPortInfo();
                if(portInfo == pMessageState.m_clientAddr.sin_port){
                    m_vecNodeBase.erase(it);
                    break;
                }
            }
        }

    }else{
        // do nothing
    }

}