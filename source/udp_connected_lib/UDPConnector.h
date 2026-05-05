#ifndef UDP_CONNECTOR_H
#define UDP_CONNECTOR_H

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>
#include "../Utils.h"


constexpr uint16_t SERVER_PORT = 12345;
constexpr int BUFFER_SIZE = 256;

/**
 * @brief Structure holding raw UDP data and client address.
 */
struct DataReceivedUDP{
    char buffer[BUFFER_SIZE];
    sockaddr_in clientAddr;
    DataReceivedUDP(){
        memset(buffer, 0, BUFFER_SIZE);
    }
};

extern ThreadSafeQueue<DataReceivedUDP> g_bufferUDPRecv;

/**
 * @brief Shared context holding the UDP socket.
 */
struct SharedContext {
    SOCKET sock;
    SharedContext() : sock(INVALID_SOCKET) {}
};


/**
 * @brief Singleton class that receives UDP packets in a dedicated thread.
 */
class UDPReceived{
public:
    UDPReceived();
    ~UDPReceived();
    bool start(SharedContext& pCtx);
    bool stop();
    void handleRxThread();
    static UDPReceived* getInstall();

private:

    std::thread m_RxThread;
    std::atomic<bool> m_running;
    std::mutex m_mtx;
    std::condition_variable m_cv;
    SharedContext m_ctx;
};

// Helper functions for UDP communication
int initUDPSock(SharedContext& pCtx);
int sendDataUDP(sockaddr_in& pTargetAddr, std::string pMessage, SharedContext& pCtx);
int recvDataUDP(char* pBuffer, sockaddr_in& pTargetAddr, SharedContext& pCtx);




#endif /* UDP_CONNECTOR_H */

