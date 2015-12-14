#include "Network.h"
#include <iostream>
#include <cassert>
#include <sstream>

#define PACKET_SIZE 2048

bool addressesEq(IPaddress a1, IPaddress a2) {
    return a1.host == a2.host && a1.port == a2.port;
}

void addrToString(const IPaddress& addr, std::string& output_str)
{
    uint32_t a = SDLNet_Read32(&addr.host);
    uint16_t p = SDLNet_Read16(&addr.port);
    uint8_t* a_ptr = (uint8_t*)&a;
    std::stringstream ss;
    ss << (int)a_ptr[3] << "." << (int)a_ptr[2] << "." << (int)a_ptr[1] << "." << (int)a_ptr[0]         // is in big endian byte order
       << ":" << p;
    output_str = ss.str();
}

Client::Client()
 : socket_(NULL), packet_(NULL)
{
    if (SDLNet_Init() == -1) {
        throw SDLNetException("Client:");
    }

    socket_ = SDLNet_UDP_Open(0);       // assigns some free port
    if (!socket_) {
        throw SDLNetException("Client:");
    }

    packet_ = SDLNet_AllocPacket(PACKET_SIZE);
    if (!packet_) {
        throw SDLNetException("Client:");
    }
}

void Client::setServer(const std::string& hostname, uint16_t port) {
    if (SDLNet_ResolveHost(&server_ip_, hostname.c_str(), port) == -1) {
        std::cerr << "Client::setServer(): " << SDLNet_GetError() << std::endl;
        return;
    }
    packet_->address = server_ip_;
}

void Client::send(void* data, uint32_t data_size) {
    assert(data_size < PACKET_SIZE);
    memcpy(packet_->data, data, data_size);
    packet_->len = data_size;
    if (SDLNet_UDP_Send(socket_, -1, packet_) == 0) {
        std::cerr << "Client::send(): " << SDLNet_GetError() << std::endl;
    }
}

void Client::recieve(const std::function<void(UDPpacket*)>& cb) {
    if (SDLNet_UDP_Recv(socket_, packet_)) {
        if (!addressesEq(packet_->address, server_ip_)) {
            // from someone else than my server -> ignore
            packet_->address = server_ip_;
            return;
        }
        cb(packet_);
    }
}

Client::~Client() {
    SDLNet_Quit();
    if (packet_)
        SDLNet_FreePacket(packet_);
}

Server::ServerSlot::ServerSlot() {
    packet_ = SDLNet_AllocPacket(PACKET_SIZE);
    if (!packet_) {
        throw SDLNetException("ServerSlot:");
    }
    packet_->address.host = INADDR_NONE;
}
Server::ServerSlot::~ServerSlot() {
    if (packet_)
        SDLNet_FreePacket(packet_);
}

bool Server::ServerSlot::isFree() {
    return packet_->address.host == INADDR_NONE;
}

Server::Server()
 : socket_(NULL), packet_(NULL)
{

}


void Server::init(uint16_t port) {
    if (SDLNet_Init() == -1) {
        throw SDLNetException("Server:");
    }

    socket_ = SDLNet_UDP_Open(port);
    if (!socket_) {
        throw SDLNetException("Server:");
    }

    packet_ = SDLNet_AllocPacket(PACKET_SIZE);
    if (!packet_) {
        throw SDLNetException("Server:");
    }
}

Server::~Server() {
    SDLNet_Quit();
    if (packet_)
        SDLNet_FreePacket(packet_);
}

void Server::update(const std::function<void(RecvCtx&)>& cb) {
    RecvCtx ctx;
    ctx.packet = packet_;
    while (SDLNet_UDP_Recv(socket_, packet_)) {
        std::string addr_str;
        addrToString(packet_->address, addr_str);
        ctx.slot_id = findClientId(packet_->address);
        if (ctx.slot_id == uint32_t(-1)) {
            ctx.new_client = true;
            ctx.slot_id = findFreeSlot();
            if (ctx.slot_id == uint32_t(-1)) {
                std::cout << "Server::update(): server full" << std::endl;
                continue;
            }
        }

        slots_[ctx.slot_id].packet_->address = packet_->address;
        cb(ctx);
    }
}

void Server::send(uint32_t slot_id, void* data, uint32_t data_size) {
    assert(data_size < PACKET_SIZE);
    assert(!slots_[slot_id].isFree());

    ServerSlot& s = slots_[slot_id];
    memcpy(s.packet_->data, data, data_size);
    s.packet_->len = data_size;
    if (SDLNet_UDP_Send(socket_, -1, s.packet_) == 0) {
        std::cerr << "Server::send(): " << SDLNet_GetError() << std::endl;
    }
}

uint32_t Server::findClientId(IPaddress ip) {
    for (uint32_t i=0; i<SLOTS_COUNT; ++i) {
        if (addressesEq(slots_[i].packet_->address, ip))
            return i;
    }
    return uint32_t(-1);
}

uint32_t Server::findFreeSlot() {
    for (uint32_t i=0; i<SLOTS_COUNT; ++i) {
        if (slots_[i].isFree())
            return i;
    }
    return uint32_t(-1);
}