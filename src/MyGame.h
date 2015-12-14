#ifndef MYGAME_H
#define MYGAME_H

#include "incl.h"

class MyGame : public Game<MyEntSys, MyGame> {
public:
    MyGame()
     : Game("Laser Sumo")
    {}

    GameEntity::IndexType game_world_id;

    GameWorld& getGameWorld();

    template <typename EType>
    void getAll(fast_vector<GameEntity*>& ents_out);

    void startServer(uint16_t port);
    void connectToServer(const std::string& hostname, uint16_t port);
    Player* getMyPlayer();
private:
    Player* findOwnedPlayer_(uint32_t owner_id, fast_vector<GameEntity*>& players);
    Vec2& getBestSpawnpoint_();

    Server server_;
    Client client_;
    uint32_t my_owner_id_;

    InputStatePacket player_input[Server::SLOTS_COUNT];
    WorldUpdatePacket* last_world_update; // update from server

    Vec2 spawnpoints[16];

    bool game_started_;

    virtual void init() override;
    virtual void update() override;

    void loadFromNetwork();
};

#include "MyGame.inl"
#endif //MYGAME_H
