/**
 * @file UDPConnector.cpp
 * @brief Implements UDP socket management and a dedicated receiver thread.
 *
 */


#include "UDPConnector.h"


#define WIN32_LEAN_AND_MEAN



UDPReceived::UDPReceived(){
    m_running.store(false,std::memory_order_release);

};

UDPReceived::~UDPReceived(){
    if(m_RxThread.joinable()){
        m_RxThread.join();
    }
};

bool UDPReceived::start(SharedContext& pCtx){
    if(!(m_running.load(std::memory_order_acquire))){
        m_running.store(true,std::memory_order_release);
        m_ctx = pCtx;
        m_RxThread = std::thread([&](){handleRxThread();});

    
        return true;

    }
    return false;
}

bool UDPReceived::stop(){

    if(m_running.load(std::memory_order_acquire)){
        m_running.store(false,std::memory_order_release);
        return true;
    }else{
        return false;
        // do nothing
    }
}

void UDPReceived::handleRxThread(){

    while(true){
        if(!m_running.load(std::memory_order_acquire)){
            
            break;
        }
        DataReceivedUDP dataUDP;
        std::unique_lock<std::mutex> lock(m_mtx);
        if(m_cv.wait_for(lock, std::chrono::milliseconds(100), [&](){return !recvDataUDP(dataUDP.buffer, dataUDP.clientAddr, m_ctx);} )){
            g_bufferUDPRecv.push(dataUDP, true);
        }
        else{
        }


    }

}

UDPReceived* UDPReceived::getInstall(){
        static UDPReceived* m_install = nullptr;
        if(m_install == nullptr){
            m_install = new UDPReceived();
        }
        
        return m_install;
}

/**
 * @brief Initializes Winsock, creates and binds a UDP socket.
 * @param pCtx Shared context that will store the created socket.
 * @return 0 on success, -1 on failure (socket closed, Winsock cleaned up).
 */
int initUDPSock(SharedContext& pCtx){
    // Initialize Winsock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        //std::cerr << "WSAStartup failed: " << result << std::endl;
        return -1;
    }

    // Create socket UDP
    pCtx.sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (pCtx.sock == INVALID_SOCKET) {
        //std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
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
        //std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
        closesocket(pCtx.sock);
        WSACleanup();
        return -1;
    }
    //std::cout << "UDP Server listening on port " << SERVER_PORT << std::endl;
    return 0;
}

/**
 * @brief Closes the UDP socket and cleans up Winsock.
 * @param pCtx Shared context containing the socket to close.
 * @return 0 on success, -1 if socket was already invalid.
 */
int closeUDPSock(SharedContext& pCtx) {
    if (pCtx.sock != INVALID_SOCKET) {
        closesocket(pCtx.sock);
        pCtx.sock = INVALID_SOCKET;
    } else {
        return -1; // Socket already closed or invalid
    }
    WSACleanup();
    return 0;
}

/**
 * @brief Sends a UDP message to a target address.
 * @param pTargetAddr Target socket address (IP and port).
 * @param pMessage String message to send.
 * @param pCtx Shared context containing the UDP socket.
 * @return 0 on success, -1 on error (invalid parameters or sendto failure).
 */
int sendDataUDP(sockaddr_in& pTargetAddr, std::string pMessage, SharedContext& pCtx){

    int targetLen;
    targetLen = sizeof(pTargetAddr);
    if(targetLen == 0 || pMessage.empty()){
        //std::cout << "[WARN] No data or client to send to yet.\n";
        return -1;
    }
    int bytesSent = sendto(pCtx.sock, pMessage.c_str(), (int)pMessage.length(), 0,
                    (sockaddr*)&pTargetAddr, targetLen);
    
    if (bytesSent == SOCKET_ERROR) {
        //std::cerr << "sendto failed: " << WSAGetLastError() << std::endl;
        return -1;
    }
    else {
        //std::cout << "[SEND] > " << pMessage << std::endl;
    }
    return 0;
}

/**
 * @brief Receives a UDP message (blocking) and stores it in a buffer.
 * @param pBuffer Output buffer to store the received message (must be at least BUFFER_SIZE).
 * @param pTargetAddr Output parameter that receives the sender's address.
 * @param pCtx Shared context containing the UDP socket.
 * @return 0 on successful reception, -1 on error (except WSAECONNRESET which is ignored).
 */
int recvDataUDP(char* pBuffer, sockaddr_in& pTargetAddr, SharedContext& pCtx){

    pTargetAddr = {};
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
            //std::cerr << "recvfrom failed with error: " << err << std::endl;
            ret = -1;
        }
        
    }else{
        // do nothing
       
    }
    return ret;
}