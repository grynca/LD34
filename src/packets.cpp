#include "incl.h"
#include <bitset>

void PlayerPacket::loadFromEntity(Player& player) {
    SDLNet_Write32(player.getOwnerClientId(), &owner_id);
    const Transform& t = player.getTransform();
    floatToNetwork(t.getPosition().getX(), &pos_x);
    floatToNetwork(t.getPosition().getY(), &pos_y);
    floatToNetwork(t.getRotation().getRads(), &rot);
    floatToNetwork(player.getMass(), &mass);

    flags = 0;
    std::bitset<8>& bs = getFlags();
    if (player.isShooting()) {
        bs[0] = true;
        floatToNetwork(player.getLaser().getSize().getX(), &laser_len);
    }
    else {
        floatToNetwork(0.0f, &laser_len);
    }
    if (player.isShieldOn())
        bs[1] = true;
    if (player.isExploding())
        bs[2] = true;
    floatToNetwork(player.getSpeed().getLinearSpeed().getX(), &l_speed_x);
    floatToNetwork(player.getSpeed().getLinearSpeed().getY(), &l_speed_y);
    floatToNetwork(player.getSpeed().getAngularSpeed().getRads(), &a_speed);
    floatToNetwork(player.getEnergy(), &energy);
}

void PlayerPacket::loadFromNetwork(void* buf) {
    PlayerPacket* p = (PlayerPacket*)buf;
    owner_id = SDLNet_Read32(&p->owner_id);
    pos_x = floatFromNetwork(&p->pos_x);
    pos_y = floatFromNetwork(&p->pos_y);
    rot = floatFromNetwork(&p->rot);
    mass = floatFromNetwork(&p->mass);
    flags = p->flags;
    laser_len = floatFromNetwork(&p->laser_len);
    l_speed_x = floatFromNetwork(&p->l_speed_x);
    l_speed_y = floatFromNetwork(&p->l_speed_y);
    a_speed = floatFromNetwork(&p->a_speed);
    energy = floatFromNetwork(&p->energy);
}


//void PlayerPacket::saveToEntity(Player& player) {
//    player.setPosition(Vec2(floatFromNetwork(&pos_x), floatFromNetwork(&pos_y)));
//    player.setRotation(Angle(floatFromNetwork(&rot)));
//    player.setScale(Vec2(floatFromNetwork(&scale_x), floatFromNetwork(&scale_y)));
//    player.getSpeed().setLinearSpeed(Vec2(floatFromNetwork(&l_speed_x), floatFromNetwork(&l_speed_y)));
//    player.getSpeed().setAngularSpeed(Angle(floatFromNetwork(&a_speed)));
//    player.mass_ = floatFromNetwork(&mass_);
//}

void GameWorldPacket::loadFromEntity(GameWorld& gw) {
    SDLNet_Write32(gw.getMassesCount(), &masses_count);
    for (uint32_t i=0; i<gw.getMassesCount(); ++i) {
        Vec2 mass_pos = gw.getMassBound(i).getCenter();

        floatToNetwork(mass_pos.getX(), &masses_x[i]);
        floatToNetwork(mass_pos.getY(), &masses_y[i]);
    }
}

void GameWorldPacket::loadFromNetwork(void* buf) {
    GameWorldPacket* p = (GameWorldPacket*)buf;
    masses_count = SDLNet_Read32(&p->masses_count);
    for (uint32_t i=0; i<masses_count; ++i) {
        masses_x[i] = floatFromNetwork(&p->masses_x[i]);
        masses_y[i] = floatFromNetwork(&p->masses_y[i]);
    }

}

InputStatePacket::InputStatePacket()
 : mouse_x(0.0f), mouse_y(0.0f), left_down(0), right_down(0)
{}

void InputStatePacket::loadFromEvents(Events& events) {
    Vec2 mp = events.getMousePos();
    floatToNetwork(mp.getX(), &mouse_x);
    floatToNetwork(mp.getY(), &mouse_y);
    left_down = uint8_t(events.isButtonDown(MouseButton::mbLeft));
    right_down = uint8_t(events.isButtonDown(MouseButton::mbRight));
}

void InputStatePacket::loadFromNetwork(void* buf, uint32_t buf_len) {
    if (buf_len != sizeof(InputStatePacket)) {
        std::cerr << "InputStatePacket::loadFromNetwork(): incorrect packet size" << std::endl;
        return;
    }
    InputStatePacket* p = (InputStatePacket*)buf;
    mouse_x = floatFromNetwork(&p->mouse_x);
    mouse_y = floatFromNetwork(&p->mouse_y);
    left_down = p->left_down;
    right_down = p->right_down;
}

Vec2 InputStatePacket::getMousePos() {
    return Vec2(mouse_x,mouse_y);
}

bool InputStatePacket::isLeftMouseDown() {
    return left_down;
}

bool InputStatePacket::isRightMouseDown() {
    return right_down;
}

void WorldUpdatePacket::loadFromEntities(GameWorld& gw, fast_vector<Player*>& players) {
    world_packet.loadFromEntity(gw);
    SDLNet_Write32(uint32_t(players.size()), &players_count);
    for (uint32_t i=0; i<players.size(); ++i) {
        player_packets[i].loadFromEntity(*players[i]);
    }
}

void WorldUpdatePacket::loadFromNetwork(void* buf, uint32_t buf_len) {
    if (buf_len != sizeof(WorldUpdatePacket)) {
        std::cerr << "WorldUpdatePacket::loadFromNetwork(): incorrect packet size" << std::endl;
        return;
    }
    WorldUpdatePacket* p = (WorldUpdatePacket*)buf;

    world_packet.loadFromNetwork(&p->world_packet);
    players_count = SDLNet_Read32(&p->players_count);
    for (uint32_t i=0; i<players_count; ++i) {
        player_packets[i].loadFromNetwork(&p->player_packets[i]);
    }
    client_id = SDLNet_Read32(&p->client_id);
}

void WorldUpdatePacket::setClientId(uint32_t id) {
    SDLNet_Write32(id, &client_id);
}