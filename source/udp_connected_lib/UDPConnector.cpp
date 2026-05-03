#include "UDPConnector.h"
// udp_server.cpp
// Build: cl /EHsc /std:c++17 udp_server.cpp ws2_32.lib
//        hoặc g++ -std=c++17 udp_server.cpp -lws2_32 -o udp_server.exe

#define WIN32_LEAN_AND_MEAN


//#pragma comment(lib, "ws2_32.lib")


int initUDPSock(SharedContext& pCtx){
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return -1;
    }

    // Create socket UDP
    pCtx.sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (pCtx.sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return -1;
    }

    int reuse = 1;
    setsockopt(pCtx.sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse));

    // Bind socket
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    serverAddr.sin_addr.s_addr = INADDR_ANY; // listen every interface

    if (bind(pCtx.sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(pCtx.sock);
        WSACleanup();
        return -1;
    }
    std::cout << "UDP Server listening on port " << SERVER_PORT << std::endl;
    return 0;
}

int sendDataUDP(sockaddr_in& pTargetAddr, std::string pMessage, SharedContext& pCtx){

    int targetLen;
    targetLen = sizeof(pTargetAddr);
    if(targetLen == 0 || pMessage.empty()){
        std::cout << "[WARN] No data or client to send to yet.\n";
        return -1;
    }
    int bytesSent = sendto(pCtx.sock, pMessage.c_str(), (int)pMessage.length(), 0,
                    (sockaddr*)&pTargetAddr, targetLen);
    
    if (bytesSent == SOCKET_ERROR) {
        std::cerr << "sendto failed: " << WSAGetLastError() << std::endl;
        return -1;
    }
    else {
        //std::cout << "[SEND] > " << pMessage << std::endl;
    }
    return 0;
}

int recvDataUDP(char* pBuffer, sockaddr_in& pTargetAddr, SharedContext& pCtx){

    int clientAddrLen = sizeof(pTargetAddr);
    char buffer[BUFFER_SIZE];
    memset(buffer, 0, BUFFER_SIZE);
    int bytesReceived = recvfrom(pCtx.sock, buffer, BUFFER_SIZE - 1, 0,
                    (sockaddr*)&pTargetAddr, &clientAddrLen);
    int ret = 0;
    if (bytesReceived > 0) {

        buffer[bytesReceived] = '\0';
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &pTargetAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        //std::cout << "[RECV] From " << clientIP << ":" << ntohs(pTargetAddr.sin_port) << " > " << buffer << std::endl;
        memcpy(pBuffer, buffer, BUFFER_SIZE);
        ret = 0;
        //std::cout << " End  recvDataUDP" << std::endl;

    }
    else if (bytesReceived == SOCKET_ERROR) {
        int err = WSAGetLastError();
        if (err != WSAECONNRESET) {
            std::cerr << "recvfrom failed with error: " << err << std::endl;
            ret = -1;
        }
        
    }else{
        // do nothing
       
    }
    return ret;
}