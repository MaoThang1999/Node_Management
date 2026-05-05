#include "Packet.h"

// ======================= Packet base class =======================
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

// ======================= RigisterPacket =======================
void RigisterPacket::setTypeNode(TypeNode pTypeNode) {
    m_typeNode = pTypeNode;
}

TypeNode RigisterPacket::getTypeNode() {
    return m_typeNode;
}

void RigisterPacket::decode(char* buffer){

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

// ======================= NotifyPacket =======================
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
    return (m_portNodeNotify);
}

void NotifyPacket::setStateNodeNotify(StateList pState) {
    m_stateNodeNotify = pState;
}

StateList NotifyPacket::getStateNodeNotify() {
    return m_stateNodeNotify;
}

std::string NotifyPacket::encode(){
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

// ======================= PingPacket =======================
std::string PingPacket::encode(){

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

// ======================= PongPacket =======================
void PongPacket::decode(char* buffer){

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