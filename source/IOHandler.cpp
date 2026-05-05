/**
 * @file IOHandler.cpp
 * @brief Implements the IOHandler singleton that manages UDP sending/receiving.
 */

#include "IOHandler.h"

// Internal queue for raw UDP data (before parsing)
ThreadSafeQueue<DataReceivedUDP> g_bufferUDPRecv;


IOHandler::IOHandler(){
    m_running.store(false,std::memory_order_release);
    m_ctx = SharedContext();

}

IOHandler::~IOHandler(){
    stop();
}

// -----------------------------------------------------------------------------
// Starts the IOHandler: initializes UDP socket, starts UDP receiver thread,
// and launches internal Tx/Rx threads.
// @return true if started successfully, false if already running.
// -----------------------------------------------------------------------------
bool IOHandler::start(){

    if(!(m_running.load(std::memory_order_acquire))){
        m_running.store(true,std::memory_order_release);
        initUDPSock(m_ctx);
        UDPReceived::getInstall()->start(m_ctx);
        g_sendIOQueue.start();
        g_bufferUDPRecv.start();

        m_RxThread = std::thread([&](){handleRxThread();});
        m_TxThread = std::thread([&](){handleTxThread();});
        return true;
    }
    return false;
}

// -----------------------------------------------------------------------------
// Stops the IOHandler: signals running flag, stops queues, and joins threads.
// @return true if stopped, false if already stopped.
// -----------------------------------------------------------------------------
bool IOHandler::stop()
{   

    if(m_running.load(std::memory_order_acquire)){

        m_running.store(false,std::memory_order_release);

        //UDPReceived::getInstall()->stop();

        g_bufferUDPRecv.stop();
        g_sendIOQueue.stop();

        if(m_RxThread.joinable()){
            m_RxThread.join();
        }
        if(m_TxThread.joinable()){
            m_TxThread.join();
        }
        return true;
    }
    return false;

}

bool IOHandler::isRunning(){
    if(m_running.load(std::memory_order_acquire)){
        return true;
    }else{
        return false;
    }
}

IOHandler* IOHandler::getInstall(){
        static IOHandler* m_install = nullptr;
        if(m_install == nullptr){
            m_install = new IOHandler();
        }
        return m_install;
}

// -----------------------------------------------------------------------------
// Tx Thread (Sending)
// Takes packets from g_sendIOQueue and sends them over UDP.
// -----------------------------------------------------------------------------
void IOHandler::handleTxThread(){
    Packet* packet = new Packet();

    while(true){

        g_sendIOQueue.waitAndPop(packet);
        if(!m_running.load(std::memory_order_acquire)){
            break;
        }
        if(packet != nullptr){


            if(packet->getMessageType() == TypeMessage::PING){

                std::string stBuffer = static_cast<PingPacket* >(packet)->encode();
                sockaddr_in clientAddr = packet->getSockaddr();
                int ret = sendDataUDP(clientAddr, stBuffer, m_ctx);
                if(ret == SUCCESS_CODE){
                    
                }else{

                }

            }else if(packet->getMessageType() == TypeMessage::NOTIFY){

                std::string stBuffer = static_cast<NotifyPacket* >(packet)->encode();
                sockaddr_in clientAddr = packet->getSockaddr();
                int ret = sendDataUDP(clientAddr, stBuffer, m_ctx);
                if(ret == SUCCESS_CODE){
                    
                }else{

                }

            }else{
                //do nothing
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
// Rx Thread (Receiving)
// Waits for raw UDP data from g_bufferUDPRecv, parses it, and pushes parsed packets
// into g_receivedIOQueue.
// -----------------------------------------------------------------------------
void IOHandler::handleRxThread(){

    DataReceivedUDP dataPDU;
    while(true){

        

        g_bufferUDPRecv.waitAndPop(dataPDU);
        if(!m_running.load(std::memory_order_acquire)){
            break;
        }

        TypeMessage typeMessage = checkMessageType(dataPDU.buffer);
        pushMessageReceived(typeMessage, dataPDU.buffer, dataPDU.clientAddr);

    }
}

// -----------------------------------------------------------------------------
// Helper: parses a raw UDP message and pushes the corresponding Packet object
// into g_receivedIOQueue.
// Note: The caller is responsible for eventually deleting the allocated Packet.
// -----------------------------------------------------------------------------
void IOHandler::pushMessageReceived(TypeMessage pTypeMessage, char* buffer, sockaddr_in pSockaddr){

    switch(pTypeMessage){
        case(TypeMessage::REGISTER):
        {
            RigisterPacket* packet = new RigisterPacket();
            packet->decode(buffer);
            packet->setSockaddr(pSockaddr);
            g_receivedIOQueue.push(packet, true);
            break;
        }
        case(TypeMessage::PONG):
        {
            PongPacket* packet = new PongPacket();
            packet->decode(buffer);
            packet->setSockaddr(pSockaddr);
            g_receivedIOQueue.push(packet, true);
            break;
        }
        default:
        // Unknown message type – ignore
            break;
    }
}
