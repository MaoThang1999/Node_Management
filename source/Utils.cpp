#include "Utils.h"


void Packet::setMessageType(TypeMessage pTypeNode) {
    m_typeMessage = static_cast<TypeMessage>(pTypeNode);
}

TypeMessage Packet::getMessageType() {
    return m_typeMessage;
}

void Packet::setMessage(std::string pMessage) {
    m_mes = std::move(pMessage);
    //m_payloadLengh = static_cast<uint8_t>(m_mes.size());
}

std::string Packet::getMessage() {
    return m_mes;
}

void Packet::setSockaddr(sockaddr_in pClientAddr) {
    m_clientAddr = pClientAddr;
}

sockaddr_in Packet::getSockaddr() {
    return m_clientAddr;
}

void Packet::setPayloadLengh(uint8_t pPayloadLengh) {
    m_payloadLengh = pPayloadLengh;
}

uint8_t Packet::getPayloadLengh() {
    return m_payloadLengh;
}

void RigisterPacket::setTypeNode(TypeNode pTypeNode) {
    m_typeNode = pTypeNode;
}

TypeNode RigisterPacket::getTypeNode() {
    return m_typeNode;
}

void RigisterPacket::unPack(char* buffer){

    int  offset = 0; 
    TypeMessage typeMessage = (TypeMessage)(get_bits(buffer, offset, 8));
    setMessageType(typeMessage);

    int payloadLengh = get_bits(buffer, offset, 8);
    setPayloadLengh(static_cast<uint8_t>(payloadLengh));

    TypeNode typeNode = (TypeNode)(get_bits(buffer, offset, 8));
    setTypeNode(typeNode);

    int size = payloadLengh - offset/8;
    if(size > 0){
        char* cMessage = new char[size + 1];

        memset(cMessage, 0, size + 1);
        get_bits(buffer, offset, cMessage, size);
        cMessage[size] = '\0';
        std::string strMessage = std::string(cMessage);
        setMessage(strMessage);
        delete[] cMessage;
    }else{
        std::string strMessage = "";
        setMessage(strMessage);
    }

}

void NotifyPacket::setTypeNode(TypeNode pTypeNode) {
    m_typeNode = pTypeNode;
}

TypeNode NotifyPacket::getTypeNode() {
    return m_typeNode;
}

void NotifyPacket::setPortNodeotify(uint16_t pPort) {
    m_portNodeNotify = pPort;
}

uint16_t NotifyPacket::getPortNodeotify() {
    return m_portNodeNotify;
}

void NotifyPacket::setStateNodeNotify(StateList pState) {
    m_stateNodeNotify = pState;
}

StateList NotifyPacket::getStateNodeNotify() {
    return m_stateNodeNotify;
}

std::string NotifyPacket::pack(){
    int offset = 0;

    char buffer[NOTIFY_PLAYLOADLENGTH + 1];
    memset(buffer, 0, NOTIFY_PLAYLOADLENGTH + 1);

    set_bits(buffer, offset, (uint32_t)(getMessageType()), 8);
    set_bits(buffer, offset, (uint32_t)(getPayloadLengh()), 8);
    set_bits(buffer, offset, (uint32_t)(getTypeNode()), 8);
    set_bits(buffer, offset, (uint32_t)(getPortNodeotify()), 16);
    set_bits(buffer, offset, (uint32_t)(getStateNodeNotify()), 8);
    std::string strMessage = getMessage();

    int sizeMess = strMessage.size();
    int sizeRemain = NOTIFY_PLAYLOADLENGTH - offset/8;
    if(sizeRemain > 0){
        if(sizeMess < sizeRemain){
            set_bits(buffer, offset, strMessage.data(), sizeMess);
        }else{
            set_bits(buffer, offset, strMessage.data(), sizeRemain - 1);
        }
    }else{
        // do nothing
    }

    buffer[NOTIFY_PLAYLOADLENGTH] = '\0';
    std::string retString = std::string(buffer, NOTIFY_PLAYLOADLENGTH);

    return retString;
}

std::string PingPacket::pack(){

    int offset = 0;

    char buffer[PING_PLAYLOADLENGTH + 1];
    memset(buffer, 0, PING_PLAYLOADLENGTH + 1);

    set_bits(buffer, offset, (uint32_t)(getMessageType()), 8);
    set_bits(buffer, offset, (uint32_t)(getPayloadLengh()), 8);

    std::string strMessage = getMessage();
    int sizeMess = strMessage.size();

    
    int sizeRemain = PING_PLAYLOADLENGTH - offset/8;
    
    if(sizeRemain > 0){
        if(sizeMess < sizeRemain){
            set_bits(buffer, offset, strMessage.data(), sizeMess);
        }else{
            set_bits(buffer, offset, strMessage.data(), sizeRemain - 1);
        }
    }else{
        // do nothing
    }


    buffer[PING_PLAYLOADLENGTH] = '\0';
    std::string retString = std::string(buffer, PING_PLAYLOADLENGTH);
    return retString;
}

void PongPacket::unPack(char* buffer){

    int  offset = 0; 
    TypeMessage typeMessage = (TypeMessage)(get_bits(buffer, offset, 8));
    setMessageType(typeMessage);

    int payloadLengh = get_bits(buffer, offset, 8);
    setPayloadLengh(static_cast<uint8_t>(payloadLengh));

    int size = payloadLengh - offset/8;
    
    if(size > 0){
        char* cMessage = new char[size + 1];

        memset(cMessage, 0, size + 1);
        get_bits(buffer, offset, cMessage, size);
        cMessage[size] = '\0';
        std::string strMessage = std::string(cMessage, size);
        setMessage(strMessage);
        delete[] cMessage;

    }else{
        std::string strMessage = "";
        setMessage(strMessage);
    }

}

TypeMessage checkMessageType(char* buffer){
    TypeMessage typeMessage;
    int offset = 0;
    typeMessage = (TypeMessage)(get_bits(buffer, offset, 8));
    return typeMessage;
}

void set_bits(char* buffer, int& offset, uint32_t value, int bitCount){
    for(int i = bitCount - 1; i >= 0; --i){
        int byteIndex = offset/8;
        int bitIndex = 7 - (offset % 8);

        buffer[byteIndex] |= ((value >> i) & 0x1) << bitIndex;
        
        offset++;
    }
}

void set_bits(char* buffer, int& offset, char* str, int strLen){
    for(int i = 0; i <strLen; ++i){
        char currentChar = str[i];
        for(int bit = 7; bit >= 0; --bit){
            int byteIndex = offset/8;
            int bitIndex = 7 - (offset % 8);

            buffer[byteIndex] |= ((currentChar >> bit) & 0x1) << bitIndex;
            ++offset;

        }

        
    }
}

int get_bits(char* buffer, int& bitOffset, int numBits){
    int value = 0;
    for(int i = 0; i < numBits; i++){
        int byteIndex = bitOffset/8;
        int bitIndex = 7 - (bitOffset % 8);

        if(buffer[byteIndex] & (1 << bitIndex)){
            value |= (1 << (numBits - 1 - i));
        }
        ++bitOffset;
    }
    return value;
}

void get_bits(char* buffer, int& bitOffset, char* str, int strLen){
    memset(str, 0, strLen);
    for(int i = 0; i < strLen; ++i){
        char cValue = 0; 
        for(int bit = 7; bit >= 0; --bit){
            int byteIndex = bitOffset/8;
            int bitIndex = 7 - (bitOffset % 8);
            if(buffer[byteIndex] & (1 << bitIndex)){
                cValue |= (1 << bit);
            }
            ++bitOffset;
        }
        str[i] = cValue;
    }
}