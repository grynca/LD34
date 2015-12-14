#include <SDL2/SDL_net.h>
#include "incl.h"

inline GameWorld& MyGame::getGameWorld() {
    return getSysEnt().getEntityManager().getItem(game_world_id).get<GameWorld>();
}

template <typename EType>
inline void MyGame::getAll(fast_vector<GameEntity*>& ents_out) {
    MyEntSys::EntityManagerType& em = getSysEnt().getEntityManager();

    for (uint32_t i=0; i<em.getItemsCount(); ++i) {
        GameEntity& e = em.getItemAtPos(i);
        if (e.is<EType>())
            ents_out.push_back(&e);
    }
}

inline void MyGame::startServer(uint16_t port) {
    std::cout << "starting server at port " << port << std::endl;
    server_.init(port);
}
inline void MyGame::connectToServer(const std::string& hostname, uint16_t port) {
    client_.setServer(hostname, port);

    std::cout << "connecting to " << hostname << " " << port << std::endl;

    // send hello packet
    std::string msg = "hello";
    client_.send((void*)msg.c_str(), msg.length());
}

inline Player* MyGame::getMyPlayer() {
    if (my_owner_id_ == uint32_t(-1))
        return NULL;

    fast_vector<GameEntity*> players;
    getAll<Player>(players);
    return findOwnedPlayer_(my_owner_id_, players);
}


inline Player* MyGame::findOwnedPlayer_(uint32_t owner_id, fast_vector<GameEntity*>& players) {
    for (uint32_t i=0; i<players.size(); ++i) {
        Player& p = players[i]->get<Player>();
        if (p.getOwnerClientId() == owner_id)
            return &p;
    }
    return NULL;
}

inline Vec2& MyGame::getBestSpawnpoint_() {
    float best_dist = 0;
    uint32_t best_sp_id = 0;

    fast_vector<GameEntity*> players;
    getAll<Player>(players);

    for (uint32_t i=0; i<16; ++i) {
        Vec2& sp = spawnpoints[i];
        float nearest_d = std::numeric_limits<float>::max();
        for (uint32_t j=0; j<players.size(); ++j) {
            Player &p = players[j]->get<Player>();
            float dist = (sp - p.getTransform().getPosition()).getSqrLen();
            if (dist < nearest_d) {
                nearest_d = dist;
            }
        }
        if (nearest_d > best_dist) {
            best_dist = nearest_d;
            best_sp_id = i;
        }
    }
    return spawnpoints[best_sp_id];
}

inline void MyGame::init() {
#ifndef WEB
    getModule<Window>().setVSync(true);
#endif
    MyAssets::init(*this);

    GameWorld::initResources();
    Player::initResources();

    game_world_id = GameWorld::create().getId();
    last_world_update = NULL;
    game_started_ = false;
    my_owner_id_ = uint32_t(-1);

    Angle a(Angle::Pi/8);
    for (uint32_t i=0; i<Server::SLOTS_COUNT; ++i) {
        spawnpoints[i] = Vec2(200, 0).rotate(a*i);
    }
//
//    GameEntity& p2 = Player::create(uint32_t(-1));
//    p2.get<Player>().setPosition(Vec2(200, 0));
//    p2.get<Player>().setControllable(false);
//    players.push_back(p2.getId());
}

inline void MyGame::update() {

    loadFromNetwork();

    Window& w = getModule<Window>();
    Events& e = w.getEvents();

    GameWorld& gw = getGameWorld();
    if (server_.isInitialized()) {
        //server
        fast_vector<GameEntity*> players;
        getAll<Player>(players);

        if (!game_started_ && players.size()>1) {
            game_started_ = true;
        }

        if (game_started_) {
            InputStatePacket input;
            input.loadFromEvents(e);
            client_.send(&input, sizeof(InputStatePacket));

            gw.updateOnServer();

            fast_vector<Player*> players2;

            // update server entities with clients' input
            for (uint32_t i = 0; i < players.size(); ++i) {
                Player &p = players[i]->get<Player>();
                p.updateOnServer(player_input[p.getOwnerClientId()]);
                players2.push_back(&p);
            }

            WorldUpdatePacket wup;
            wup.loadFromEntities(gw, players2);

            // send world update to clients
            for (uint32_t i=0; i<server_.SLOTS_COUNT; ++i) {
                Server::ServerSlot& slot = server_.getSlot(i);
                wup.setClientId(i);
                if (!slot.isFree()) {
                    server_.send(i, &wup, sizeof(WorldUpdatePacket));
                }
            }
        }
    }
    else if (last_world_update) {
        // client
        InputStatePacket input;
        input.loadFromEvents(e);
        client_.send(&input, sizeof(InputStatePacket));

        getGameWorld().updateOnClient(last_world_update->world_packet);

        fast_vector<GameEntity*> players;
        getAll<Player>(players);
        for (uint32_t i=0; i<last_world_update->players_count; ++i) {
            PlayerPacket& pp = last_world_update->player_packets[i];
            Player* p = findOwnedPlayer_(pp.owner_id, players);
            if (!p) {
                p = &Player::create(pp.owner_id).get<Player>();  // add new player entity
                p->setPosition(Vec2(pp.pos_x, pp.pos_y));
                p->setRotation(Angle(pp.rot));
            }
            p->updateOnClient(pp);
        }
    }
}

inline void MyGame::loadFromNetwork() {
    if (server_.isInitialized()) {
        server_.update([this](Server::RecvCtx& rctx) {
            if (rctx.new_client) {
                std::cout << "creating entity for new player: " << rctx.slot_id << std::endl;
                // create player entity for newly connected client
                Vec2 sp = getBestSpawnpoint_();
                GameEntity& p1 = Player::create(rctx.slot_id);
                Player& p = p1.get<Player>();
                p.setPosition(sp);
                p.setRotation(sp.getAngle()+Angle::Pi);
            }
            else {
                player_input[rctx.slot_id].loadFromNetwork(rctx.packet->data, uint32_t(rctx.packet->len));
            }
        });
        client_.recieve([this](UDPpacket* packet) {
            WorldUpdatePacket wup;
            wup.loadFromNetwork(packet->data, uint32_t(packet->len));
            my_owner_id_ = wup.client_id;
        });
    }
    else {
        // just client -> load game update from network
        client_.recieve([this](UDPpacket* packet) {
            if (!last_world_update) {
                last_world_update = new WorldUpdatePacket();
            }
            last_world_update->loadFromNetwork(packet->data, uint32_t(packet->len));
            my_owner_id_ = last_world_update->client_id;
        });
    }
}