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

constexpr uint16_t SERVER_PORT = 12345;
constexpr int BUFFER_SIZE = 256;

struct SharedContext {
    SOCKET sock;
    std::mutex mtx;
    std::atomic<bool> running;

    SharedContext() : sock(INVALID_SOCKET), running(true) {}
};

int initUDPSock(SharedContext& pCtx);
int sendDataUDP(sockaddr_in& pTargetAddr, std::string pMessage, SharedContext& pCtx);
int recvDataUDP(char* pBuffer, sockaddr_in& pTargetAddr, SharedContext& pCtx);




#endif /* UDP_CONNECTOR_H */

