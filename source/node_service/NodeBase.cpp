/**
 * @file NodeBase.cpp
 * @brief Implements the base node state machine with ping/pong health checking.
 */

#include "NodeBase.h"


NodeBase::NodeBase(){
    createID();
    m_pongState.store(false,std::memory_order_release);
    m_running.store(false,std::memory_order_release);
    m_type = TypeNode::BASE;
    m_objectNodeID = m_nodeID;
}
int NodeBase::m_nodeID = 0;

NodeBase::~NodeBase(){
    if(m_thread.joinable()){
        m_thread.join();
    }
}

void NodeBase::start(){
    if( !(m_running.load(std::memory_order_acquire)) ){
        m_running.store(true,std::memory_order_release);
        std::unique_ptr<NodeState> state(new InitState());
        m_currentState = std::move(state);
        m_thread = std::thread([&](){handleThread();});

    }
}

void NodeBase::stop(){
    std::lock_guard<std::mutex> lock(m_mtx);
    if(m_running.load(std::memory_order_acquire)){
        m_running.store(false,std::memory_order_release);
        m_cv.notify_all();
    }else{
        // do nothing
    }

}

// -----------------------------------------------------------------------------
// Main loop: repeatedly calls the current state's handleState until stopped.
// -----------------------------------------------------------------------------
void NodeBase::handleThread(){
    while(true){
        if(!m_running.load(std::memory_order_acquire)){
            break;
        }
        if(m_currentState != nullptr){
            try{
                m_currentState->handleState(this);
            }
            // Log error, but continue running
            catch (const std::exception& e) {
                DEBUG_LOG("Exception caught: " << e.what() << std::endl);
            }
            catch (...) {
                DEBUG_LOG("Unknown exception caught" << std::endl);
            }
            
        }


    }
}

// -----------------------------------------------------------------------------
// Changes the node's state only if the transition is allowed by g_stateChangeTable.
// -----------------------------------------------------------------------------
void NodeBase::changeState(std::unique_ptr<NodeState> p_state){

    if(p_state == nullptr){
        return;
    }
    if(g_stateChangeTable[(int)m_currentState->getState()][(int)p_state->getState()]){
        m_currentState = std::move(p_state);
        
    }else{
        // Couldn't change state
    }
    
}

void NodeBase::createID(){
    m_nodeID++;
}

// -----------------------------------------------------------------------------
// @return The unique ID of this node instance.
// -----------------------------------------------------------------------------
int NodeBase::getObjectNodeID(){
    return m_objectNodeID;
}

TypeNode NodeBase::getType(){
    return m_type;
}

std::string NodeBase::getNameType(){

    std::string retMess = "";
    switch (m_type)
    {
    case TypeNode::DATA_PLANE:
    {
        retMess = "DATA_PLANE";
        break;
    }
    case TypeNode::CONTROL_PLANE:
    {
        retMess = "CONTROL_PLANE";
        break;
    }
    case TypeNode::OAM:
    {
        retMess = "OAM";
        break;
    }
    default:
        break;
    }
    return retMess;
}

// -----------------------------------------------------------------------------
// Sends a PING message to the NodeManagament via the global message queue.
// -----------------------------------------------------------------------------
void NodeBase::sendPing(){
    // push message ping to g_messageNodeQueue to NoM check
    str_MessageState mesNodeToNoM(m_nodeID, m_type, m_currentState->getState());
    mesNodeToNoM.m_messageType = 0;
    mesNodeToNoM.m_clientAddr = getClientInfor();
    mesNodeToNoM.m_message = "Ping";
    g_messageNodeQueue.push(mesNodeToNoM, true);

}

// -----------------------------------------------------------------------------
// @return Current state (INIT, ALIVE, TIMEOUT, DEAD).
// -----------------------------------------------------------------------------
StateList NodeBase::getState(){
    return m_currentState->getState();
}

// -----------------------------------------------------------------------------
// Sets the pong flag and notifies waiting threads.
// The mutex is locked to avoid lost wakeup with condition_variable.
// -----------------------------------------------------------------------------
void NodeBase::setPongState(bool pFlag){

    std::lock_guard<std::mutex> lock(m_mtx);
    m_pongState.store(pFlag, std::memory_order_release);
    m_cv.notify_all();
}
bool NodeBase::getPongState(){
    return m_pongState.load(std::memory_order_acquire);
}

void NodeBase::setClientInfor(sockaddr_in pClientAddr){
    m_clientAddr = std::move(pClientAddr);
}

uint16_t NodeBase::getPortInfo(){
    return m_clientAddr.sin_port;
}

sockaddr_in NodeBase::getClientInfor(){
    return m_clientAddr;
}

void NodeState::handleState(NodeBase* node){
    if(node == nullptr){
       return;
    }
}

// =============================================================================
// INIT STATE
// =============================================================================
InitState::InitState(){
    m_state = StateList::INIT;
}

void InitState::handleState(NodeBase* node) {
    if(node == nullptr){
        return;
    }
    node->setPongState(false);
    node->sendPing();
    std::unique_lock<std::mutex> lock(node->m_mtx);
    // If timeout, return false. If pongState is true or m_running is false, return true.
    if(node->m_cv.wait_for(lock, std::chrono::milliseconds(TIMEOUT_PERIODS), [&](){return node->getPongState() || !(node->m_running.load(std::memory_order_acquire));} )){
        if( !node->m_running.load(std::memory_order_acquire)){
            // Node is stopping – do nothing, main loop will break
            return;
        }else{
            // Pong received – move to ALIVE
            std::unique_ptr<NodeState> state(new AliveState());
            node->changeState(std::move(state));

        }
    }else{
        // Timeout – move to TIMEOUT
        std::unique_ptr<NodeState> state(new TimeoutState());
        node->changeState(std::move(state));
    }

}

// =============================================================================
// ALIVE STATE
// =============================================================================
AliveState::AliveState(){
    m_state = StateList::ALIVE;
}

void AliveState::handleState(NodeBase* node) {
    if(node == nullptr){
        return;
    }

    node->setPongState(false);
    node->sendPing();
    std::unique_lock<std::mutex> lock(node->m_mtx);
    // If timeout, return false. If pongState is true or m_running is false, return true.
    if(node->m_cv.wait_for(lock, std::chrono::milliseconds(TIMEOUT_PERIODS), [&](){return node->getPongState() || !(node->m_running.load(std::memory_order_acquire));} )){
        if( !node->m_running.load(std::memory_order_acquire)){
            std::unique_ptr<NodeState> state(new InitState());
            node->changeState(std::move(state));
            // Stopping – no state change needed
            return;
        }else{
            // Pong received – stay alive, wait before next ping
            std::this_thread::sleep_for(std::chrono::milliseconds(TIMESEND_PING));
            // remains in AliveState
        }
    }else{
        // Timeout – go to TIMEOUT
        std::unique_ptr<NodeState> state(new TimeoutState());
        node->changeState(std::move(state));
    }

}

// =============================================================================
// TIMEOUT STATE
// =============================================================================
TimeoutState::TimeoutState(){
    m_state = StateList::TIMEOUT;
}

void TimeoutState::handleState(NodeBase* node) {
    if(node == nullptr){
        return;
    }
    int nodeID = node->getObjectNodeID();
    std::string typeIDMess = node->getNameType();
    uint16_t port = ntohs(node->getPortInfo());

    DEBUG_LOG("NodeID[" << nodeID << "]" << " Type[" << typeIDMess << "] Port[" << port << "] timeout");

    // Notify NoM about timeout (once)
    str_MessageState mesTimeout(node->getObjectNodeID(), node->getType(), node->getState(), 1U);
    mesTimeout.m_messageType = 1; // NOTIFY
    mesTimeout.m_clientAddr = node->getClientInfor();
    mesTimeout.m_message = "Timeout ";
    mesTimeout.saveTime();
    g_messageNodeQueue.push(mesTimeout,true);

    uint8_t retryTime = 0;
    while(retryTime < RETRY_NUM){
        node->setPongState(false);
        node->sendPing();
        std::unique_lock<std::mutex> lock(node->m_mtx);
        // If timeout, return false. If pongState is true or m_running is false, return true.
        if(node->m_cv.wait_for(lock, std::chrono::milliseconds(TIMEOUT_PERIODS), [&](){return node->getPongState() || !(node->m_running.load(std::memory_order_acquire));} )){
            
            if( !node->m_running.load(std::memory_order_acquire)){
                // Stopping – go to INIT (thread will exit anyway)
                std::unique_ptr<NodeState> state(new InitState());
                node->changeState(std::move(state));
                return;
            }else{
                // Pong received – back to ALIVE
                std::unique_ptr<NodeState> state(new AliveState());
                node->changeState(std::move(state));
                std::this_thread::sleep_for(std::chrono::milliseconds(TIMESEND_PING));
                DEBUG_LOG("NodeID[" << nodeID << "]" << " Type[" << typeIDMess << "] Port[" << port << "] Alive");
                return;                
            }
            
        }else{
            // Continue sending pings until the retry number is reached.
        }
        retryTime++;
    }
    if(retryTime == RETRY_NUM){
        // All retries failed – node is DEAD
        std::unique_ptr<NodeState> state(new DeadState());
        node->changeState(std::move(state));
        return;

    }

}

// =============================================================================
// DEAD STATE
// =============================================================================
DeadState::DeadState(){
    m_state = StateList::DEAD;
}

void DeadState::handleState(NodeBase* node) {
    if(node == nullptr){
        return;
    }
    int nodeID = node->getObjectNodeID();
    std::string typeIDMess = node->getNameType();
    uint16_t port = ntohs(node->getPortInfo());

    DEBUG_LOG("NodeID[" << nodeID << "]" << " Type[" << typeIDMess << "] Port[" << port << "] dead");

    // Notify for NoM that this node is dead, need to remove
    str_MessageState mesTimeout(node->getObjectNodeID(), node->getType(), node->getState(), 1U);
    mesTimeout.saveTime();
    mesTimeout.m_messageType = 1;
    mesTimeout.m_clientAddr = node->getClientInfor();
    g_messageNodeQueue.push(mesTimeout, true);

    std::unique_ptr<NodeState> state(new InitState());
    node->changeState(std::move(state));
    
    // Stop the node – the main thread will exit
    node->stop();

    return;
}
