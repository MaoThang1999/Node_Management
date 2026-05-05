#ifndef PACKET_H
#define PACKET_H

#include "Utils.h"

/**
 * @brief Base class for all protocol packets.
 * Provides common fields: message type, payload length, message string, and client address.
 */
class Packet{
public:

    virtual ~Packet() = default;
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

/**
 * @brief Register packet sent by a node to announce itself.
 * Contains node type (DATA_PLANE, CONTROL_PLANE, OAM).
 */
class RigisterPacket : public Packet{
public:
    void setTypeNode(TypeNode pTypeNode);
    TypeNode getTypeNode();
    void decode(char* buffer); // Populates fields from raw buffer

private:
    TypeNode m_typeNode;

};

/**
 * @brief Ping packet – sent by the manager to a node to check liveness.
 */
class PingPacket : public Packet{
public:
    std::string encode(); // Serializes packet into a string usable for UDP send
};

/**
 * @brief Pong packet – sent by a node in response to a Ping.
 */
class PongPacket : public Packet{
public:
    void decode(char* buffer);
};

/**
 * @brief Notify packet – used to broadcast state changes (timeout/dead) to other nodes.
 */
class NotifyPacket : public Packet{
public:
    void setTypeNode(TypeNode pTypeNode);
    TypeNode getTypeNode();
    void setPortNodeotify(uint16_t pPort);
    uint16_t getPortNodeotify();
    void setStateNodeNotify(StateList pState);
    StateList getStateNodeNotify();
    std::string encode();  // Serializes into buffer according to protocol
private:
    TypeNode m_typeNode;
    uint16_t m_portNodeNotify;
    StateList m_stateNodeNotify;

};


#endif /* PACKET_H */