#ifndef IO_HANDLE_H
#define IO_HANDLE_H

#include <string>
#include <vector>
#include "Utils.h"
#include "udp_connected_lib\UDPConnector.h"
#include "PacKet.h"




extern ThreadSafeQueue<Packet*> g_sendIOQueue;
extern ThreadSafeQueue<Packet*> g_receivedIOQueue;



class IOHandler{
public:
    IOHandler();
    ~IOHandler();
    bool start();
    bool stop();
    bool isRunning();
    void handleTxThread(); // Sends packets from g_sendIOQueue over UDP
    void handleRxThread(); // Receives UDP packets and pushes parsed packets to g_receivedIOQueue
    static IOHandler* getInstall();

private:

    std::thread m_RxThread;
    std::thread m_TxThread;
    std::atomic<bool> m_running;
    std::condition_variable m_cv;
    std::mutex m_mtx;
    SharedContext m_ctx; // UDP socket context
    void pushMessageReceived(TypeMessage pTypeMessage, char* buffer, sockaddr_in pSockaddr);
    

};
#endif /* IO_HANDLE_H */