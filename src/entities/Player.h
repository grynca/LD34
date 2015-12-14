#ifndef PLAYER_H
#define PLAYER_H

class InputStatePacket;

class Player : public EMovable,
               public ERenderables
{
public:
    struct Target {
        Target() : target_id(VersionedIndex::Invalid()) {}

        VersionedIndex target_id;
        OverlapInfo overlap_info;
    };

    enum ShieldState {
        ssOff,
        ssGoingOn,
        ssOn,
        ssGoingOf
    };

    Player(uint32_t owner_id);

    static void initResources();
    static GameEntity& create(uint32_t owner_id);
    void updateOnServer(InputStatePacket& client_input);
    void updateOnClient(PlayerPacket& player_packet);
    bool isShooting();
    bool isExploding() { return exploding_; }
    Target& getLaserTarget() { return laser_target_; }
    Circle getBodyBound();
    Circle getShieldBound();
    float getMass() { return mass_; }
    float getMassScaleFactor();
    bool isShieldOn();
    float getEnergy() { return energy_; }
    static float getMaxEnergy();

    uint32_t getOwnerClientId() { return owner_client_id_; }
private:
    friend class PlayerPacket;

    CircleRenderable& getBody();
    SpriteRenderable& getArrow();
    RectRenderable& getLaser();
    SpriteRenderable& getShield();
    SpriteRenderable& getExplosion();

    Ray getLaserRay();

    Vec2 getForwardDir();

    void setMass(float mass);
    void beginExploding();
    void updateEnergy();

    static struct Resources {
        const TextureRegion* arrow;
        uint32_t shield_on_anim;
        uint32_t shield_anim;
        uint32_t explosion_anim;
    } resources_;
    float mass_;
    float energy_;
    uint32_t owner_client_id_;
    VersionedIndex my_id_;
    ShieldState shield_state_;
    Target laser_target_;
    bool exploding_;
};


#endif //PLAYER_H
