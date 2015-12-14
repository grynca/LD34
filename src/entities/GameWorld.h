#ifndef GAMEWORLD_H
#define GAMEWORLD_H

class GameWorld : public ETransform,
                  public ERenderables {
public:
    GameWorld();

    static void initResources();
    static GameEntity& create();
    void updateOnServer();
    void updateOnClient(GameWorldPacket& gwp);

    static float getWorldRadius();

    uint32_t getMassesCount();
    Circle getMassBound(uint32_t id);
    void removeMass(uint32_t id);

private:
    friend class GameWorldPacket;

    RectRenderable& getEnergyBar_();

    static struct Resources {
        const TextureRegion& background;
    } resources_;

    void spawnMass_(MyGame& g);
    void addMass_(const Vec2& pos);
    void clearMasses_();

    uint32_t to_next_mass_;
};

#endif //GAMEWORLD_H
