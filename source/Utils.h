#ifndef UTILS_H
#define UTILS_H


#include <mutex>
#include <condition_variable>
#include <queue>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <memory>
#include <chrono>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <atomic>
#include <stdio.h>
#include "Log_Handler.h"

#define MAX_SAFE_QUEUE 512
#define ERROR_CODE            (-1)
#define SUCCESS_CODE            (0)
#define MAX_STATE 4 
#define TIMEOUT_PERIODS 2000
#define TIMESEND_PING   500
#define RETRY_NUM 5



constexpr uint8_t PING_PLAYLOADLENGTH = 20;
//constexpr uint8_t PONG_PLAYLOADLENGTH = 10;
//constexpr uint8_t RIGISTER_PLAYLOADLENGTH = 15;
constexpr uint8_t NOTIFY_PLAYLOADLENGTH = 40;

enum class TypeNode {BASE, DATA_PLANE, CONTROL_PLANE, OAM};
enum class TypeMessage {REGISTER, PING, PONG, NOTIFY, OTHER};
enum class StateList {INIT, ALIVE, TIMEOUT, DEAD, MAX};

struct str_MessageState{
    str_MessageState(int pNodeID = 0 , TypeNode pTypeID = TypeNode::BASE , StateList pState = StateList::INIT, uint8_t pMessageType = 0 ){
        m_nodeID = pNodeID;
        m_typeNode = pTypeID;
        m_state = pState;
        m_messageType = pMessageType;
    }
    void saveTime(){
        auto now = std::chrono::system_clock::now();
        std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
        // Convert local time
        std::tm now_tm = *std::localtime(&now_time_t);
        // Forrmat YYYY-MM-DD HH:MM:SS
        std::ostringstream oss;
        oss << std::put_time(&now_tm, "%Y-%m-%d %H:%M:%S");
        m_message += oss.str();
    }

    int m_nodeID;
    TypeNode m_typeNode;
    std::string m_message;
    StateList m_state;
    uint8_t m_messageType; // 0 : ping message, 1: changeState message
    sockaddr_in m_clientAddr;
};

template <typename T>
class ThreadSafeQueue{

public:
    ~ThreadSafeQueue() = default;

    void push(T value, bool bForce)
    {
        std::lock_guard<std::mutex> lock(m_mtx);
        if(m_queue.size() >= MAX_SAFE_QUEUE){
            if(bForce){
                m_queue.pop();
            }else{
                return;
            }
        }
        m_queue.push(std::move(value));
        m_cv.notify_all();
    }

    bool peek(T& value){
        std::lock_guard<std::mutex> lock(m_mtx);
        if(m_queue.empty()){
            return false;
        }else{
            value = m_queue.front();
            return true;
        }
    }

    bool pop(){
        std::lock_guard<std::mutex> lock(m_mtx);
        if(m_queue.empty()){
            return false;
        }else{
            m_queue.pop();
            return true;
        }

    }
    
    void resetQueue(){
        std::lock_guard<std::mutex> lock(m_mtx);
        m_queue = {};  
    }

    bool tryPop(T &value){
        std::lock_guard<std::mutex> lock(m_mtx);
        if(m_queue.empty()){
            return false;
        }else{
            value = std::move(m_queue.front());
            m_queue.pop();
            return true;
        }        
    }

    int getSize(){
        std::lock_guard<std::mutex> lock(m_mtx);
        return m_queue.size();
    }

    void waitAndPop(T &value, std::atomic<bool>& pRunning){
        std::unique_lock<std::mutex> lock(m_mtx);
        m_cv.wait(lock, [&]{return !m_queue.empty() || !pRunning.load(std::memory_order_acquire);});
        if(!m_queue.empty()){
            value = std::move(m_queue.front());
            m_queue.pop();
        }
    }

private:
    std::queue<T> m_queue;
    std::mutex m_mtx;
    std::condition_variable m_cv;

};

class Packet{
public:

    void setMessageType(TypeMessage pTypeNode);
    TypeMessage getMessageType();
    void setMessage(std::string pMessage);
    std::string getMessage();
    void setSockaddr(sockaddr_in pClientAddr);
    sockaddr_in getSockaddr();
    void setPayloadLengh(uint8_t pPayloadLengh);
    uint8_t getPayloadLengh();
private:
    TypeMessage m_typeMessage;
    uint8_t m_payloadLengh;
    std::string m_mes;
    sockaddr_in m_clientAddr;
};

class RigisterPacket : public Packet{
public:
    void setTypeNode(TypeNode pTypeNode);
    TypeNode getTypeNode();
    void unPack(char* buffer);

private:
    TypeNode m_typeNode;

};

class PingPacket : public Packet{
public:
    std::string pack();
};

class PongPacket : public Packet{
public:
    void unPack(char* buffer);
};

class NotifyPacket : public Packet{
public:
    void setTypeNode(TypeNode pTypeNode);
    TypeNode getTypeNode();
    void setPortNodeotify(uint16_t pPort);
    uint16_t getPortNodeotify();
    void setStateNodeNotify(StateList pState);
    StateList getStateNodeNotify();
    std::string pack();
private:
    TypeNode m_typeNode;
    uint16_t m_portNodeNotify;
    StateList m_stateNodeNotify;

};


TypeMessage checkMessageType(char* buffer);

void set_bits(char* buffer, int& offset, uint32_t value, int bitCount);
void set_bits(char* buffer, int& offset, char* str, int strLen);
int get_bits(char* buffer, int& bitOffset, int numBits);
void get_bits(char* buffer, int& bitOffset, char* str, int strLen);


#endif /* UTILS_H */
