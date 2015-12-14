#ifndef PACKETS_H
#define PACKETS_H

#include <SDL2/SDL_net.h>
#define MAX_MASSES 50

// fw
class Player;
class GameWorld;

static void floatToNetwork(float value, void* area) {
    SDLNet_Write32(*(uint32_t*)&value, area);
}
static float floatFromNetwork(void* area) {
    uint32_t tmp = SDLNet_Read32(area);
    return *(float*)&tmp;
}

struct PlayerPacket {
public:
    void loadFromEntity(Player& player);
    void loadFromNetwork(void* buf);

    std::bitset<8>& getFlags() { return *(std::bitset<8>*)&flags;}
    uint32_t owner_id;
    float pos_x, pos_y;
    float rot;
    float mass;
    float laser_len;
    float l_speed_x, l_speed_y;
    float a_speed;
    float energy;
    uint8_t flags;
};

struct GameWorldPacket {
public:
    void loadFromEntity(GameWorld& gw);
    void loadFromNetwork(void* buf);

    uint32_t masses_count;
    float masses_x[MAX_MASSES];
    float masses_y[MAX_MASSES];
};

struct InputStatePacket {
public:
    InputStatePacket();

    void loadFromEvents(Events& events);
    void loadFromNetwork(void* buf, uint32_t buf_len);
    Vec2 getMousePos();
    bool isLeftMouseDown();
    bool isRightMouseDown();
private:
    float mouse_x, mouse_y;
    uint8_t left_down, right_down;
};

struct WorldUpdatePacket {
    void loadFromEntities(GameWorld& gw, fast_vector<Player*>& players);
    void loadFromNetwork(void* buf, uint32_t buf_len);
    void setClientId(uint32_t id);

    GameWorldPacket world_packet;
    PlayerPacket player_packets[Server::SLOTS_COUNT];
    uint32_t players_count;
    uint32_t client_id;
};

#endif //PACKETS_H
