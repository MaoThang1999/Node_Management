#ifndef IO_HANDLE_H
#define IO_HANDLE_H

#include "Utils.h"
#include "udp_connected_lib\UDPConnector.h"
#include <string>
#include <vector>



extern ThreadSafeQueue<Packet*> g_sendIOQueue;
extern ThreadSafeQueue<Packet*> g_receivedIOQueue;



class IOHandler{
public:
    IOHandler();
    ~IOHandler();
    void start();
    void stop();
    void resume();
    void handleTxThread();
    void handleRxThread();
    static IOHandler* getInstall();

private:

    std::thread m_RxThread;
    std::thread m_TxThread;
    std::atomic<bool> m_running;
    std::condition_variable m_cv;
    std::mutex m_mtx;
    SharedContext m_ctx;
    void pushMessageReceived(TypeMessage pTypeMessage, char* buffer, sockaddr_in pSockaddr);
    

};
#endif /* IO_HANDLE_H */