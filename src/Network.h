#ifndef NETWORK_H
#define NETWORK_H

#include <SDL2/SDL_net.h>
#include <stdexcept>
#include <functional>

struct SDLNetException : public std::runtime_error
{
    SDLNetException(const std::string& msg = std::string()) throw()
            : std::runtime_error(msg + ",\n  SDLNet error: " + SDLNet_GetError())
    {}
};

static bool addressesEq(IPaddress a1, IPaddress a2);
static void addrToString(const IPaddress& addr, std::string& output_str);

class Client {
public:
    Client();
    ~Client();

    void setServer(const std::string& hostname, uint16_t port);

    void send(void* data, uint32_t data_size);
    void recieve(const std::function<void(UDPpacket*)>& cb);

    IPaddress getServerAddress() { return server_ip_; }
private:
    IPaddress server_ip_;
    UDPsocket socket_;
    UDPpacket* packet_;
};


class Server {
public:
    static constexpr int SLOTS_COUNT = 16;

    struct ServerSlot {
        ServerSlot();
        ~ServerSlot();

        bool isFree();
        IPaddress getClientAddress() { return packet_->address; }

        UDPpacket *packet_;
    };

    struct RecvCtx {
        uint32_t slot_id;
        bool new_client;
        UDPpacket* packet;
    };

public:
    Server();
    ~Server();

    void init(uint16_t port);

    void update(const std::function<void(RecvCtx&)>& cb);

    void send(uint32_t slot_id, void* data, uint32_t data_size);

    bool isInitialized() { return socket_!=NULL; }

    ServerSlot& getSlot(uint32_t i) { return slots_[i]; }
private:

    uint32_t findClientId(IPaddress ip);
    uint32_t findFreeSlot();

    UDPsocket socket_;
    UDPpacket* packet_;

    ServerSlot slots_[SLOTS_COUNT];
};


#endif //NETWORK_H
