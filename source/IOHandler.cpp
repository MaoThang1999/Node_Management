#include "IOHandler.h"

IOHandler::IOHandler(){
    m_running.store(false,std::memory_order_release);
    SharedContext m_ctx;

}

IOHandler::~IOHandler(){
    stop();
}

void IOHandler::start(){
    if(!(m_running.load(std::memory_order_acquire))){
        m_running.store(true,std::memory_order_release);
        initUDPSock(m_ctx);
        m_RxThread = std::thread([&](){handleTxThread();});
        m_TxThread = std::thread([&](){handleRxThread();});
        
    }
}

void IOHandler::stop()
{
    std::lock_guard<std::mutex> lock(m_mtx);
    if(m_running.load(std::memory_order_acquire)){
        m_running.store(false,std::memory_order_release);
        m_cv.notify_all();
        if(m_RxThread.joinable()){
            m_RxThread.join();
        }
        if(m_TxThread.joinable()){
            m_TxThread.join();
        }
    }
    

}

void IOHandler::resume()
{
    std::lock_guard<std::mutex> lock(m_mtx);
    if(!(m_running.load(std::memory_order_acquire))){
        m_running.store(true,std::memory_order_release);
        m_RxThread = std::thread([&](){handleTxThread();});
        m_TxThread = std::thread([&](){handleRxThread();});
        std::cout << "UDP Server listening on port " << SERVER_PORT << std::endl;
        m_cv.notify_all();
    }

}

IOHandler* IOHandler::getInstall(){
        static IOHandler* m_install = nullptr;
        if(m_install == nullptr){
            m_install = new IOHandler();
        }
        return m_install;
}

void IOHandler::handleTxThread(){
    Packet* packet = new Packet();

    while(true){

        g_sendIOQueue.waitAndPop(packet, m_running);

        if(!m_running.load(std::memory_order_acquire)){
            break;
        }
        if(packet->getMessageType() == TypeMessage::PING){

            std::string stBuffer = static_cast<PingPacket* >(packet)->pack();
            sockaddr_in clientAddr = packet->getSockaddr();
            int ret = sendDataUDP(clientAddr, stBuffer, m_ctx);
            if(ret == SUCCESS_CODE){
                
            }else{

            }

        }else if(packet->getMessageType() == TypeMessage::NOTIFY){

            std::string stBuffer = static_cast<NotifyPacket* >(packet)->pack();
            sockaddr_in clientAddr = packet->getSockaddr();
            int ret = sendDataUDP(clientAddr, stBuffer, m_ctx);
            if(ret == SUCCESS_CODE){
                
            }else{

            }

        }else{
            //do nothing
        }
    }
}

void IOHandler::handleRxThread(){

    sockaddr_in clientAddr;
    char buffer[BUFFER_SIZE];
    
    while(true){

        clientAddr = {};
        memset(buffer, 0, BUFFER_SIZE);

        std::unique_lock<std::mutex> lock(m_mtx);
        m_cv.wait(lock, [&]{return !m_running.load(std::memory_order_acquire) || recvDataUDP(buffer,clientAddr, m_ctx) == SUCCESS_CODE;});
        if(!m_running.load(std::memory_order_acquire)){
            break;
        }
        TypeMessage typeMessage = checkMessageType(buffer);
        pushMessageReceived(typeMessage, buffer, clientAddr);



    }
}

void IOHandler::pushMessageReceived(TypeMessage pTypeMessage, char* buffer, sockaddr_in pSockaddr){

    switch(pTypeMessage){
        case(TypeMessage::REGISTER):
        {
            RigisterPacket* packet = new RigisterPacket();
            packet->unPack(buffer);
            packet->setSockaddr(pSockaddr);
            g_receivedIOQueue.push(packet, true);
            break;
        }
        case(TypeMessage::PONG):
        {
            PongPacket* packet = new PongPacket();
            packet->unPack(buffer);
            packet->setSockaddr(pSockaddr);
            g_receivedIOQueue.push(packet, true);
            break;
        }
        default:
            break;
    }
}
