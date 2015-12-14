#include "../incl.h"

#define INITIAL_BODY_RADIUS 25.0f
#define MOVEMENT_SPEED 100.0f
#define ROTATION_SPEED 3.14f
#define MAX_LASER_LEN 5000.0f
#define LASER_PUSH_FORCE 100.0f
#define LASER_ROT_FORCE 10.0f
#define INITIAL_MASS 5.0f
#define MASS_INCREMENT 1.0f
#define SHIELD_INTERNAL_SCALE 0.75f
#define EXPLOSION_INTERNAL_SCALE 2.0f
#define MAX_ENERGY 100.0f
#define LASER_DRAIN 30.f/60.0f
#define SHIELD_DRAIN 30.f/60.0f
#define ENERGY_RECHARGE 15.f/60.0f

Player::Resources Player::resources_;

Player::Player(uint32_t owner_id)
 : mass_(INITIAL_MASS), energy_(MAX_ENERGY), owner_client_id_(owner_id), shield_state_(ssOff), exploding_(false)
{
}

void Player::initResources() {
    MyGame& g = MyGame::get();
    AssetsManager& am = g.getModule<AssetsManager>();
    resources_.arrow = am.getImageRegion("data/sprites/arrow.png");

    Animation& shield_anim = am.getAnimations().addItem();
    resources_.shield_anim = shield_anim.getId();
    shield_anim.init({
                      {"data/sprites/shield_anim/sa-0.png", 0.08f},
                      {"data/sprites/shield_anim/sa-1.png", 0.08f},
                      {"data/sprites/shield_anim/sa-2.png", 0.08f},
                      {"data/sprites/shield_anim/sa-3.png", 0.08f},
                      {"data/sprites/shield_anim/sa-4.png", 0.08f},
                      {"data/sprites/shield_anim/sa-5.png", 0.08f},
                      {"data/sprites/shield_anim/sa-6.png", 0.08f},
                      {"data/sprites/shield_anim/sa-7.png", 0.08f},
                      {"data/sprites/shield_anim/sa-8.png", 0.08f},
                      {"data/sprites/shield_anim/sa-9.png", 0.08f},
                      {"data/sprites/shield_anim/sa-10.png", 0.08f},
                      {"data/sprites/shield_anim/sa-11.png", 0.08f},
                      {"data/sprites/shield_anim/sa-12.png", 0.08f},
                      {"data/sprites/shield_anim/sa-13.png", 0.08f},
                      {"data/sprites/shield_anim/sa-14.png", 0.08f},
                      {"data/sprites/shield_anim/sa-15.png", 0.08f},
                      {"data/sprites/shield_anim/sa-16.png", 0.08f},
                      {"data/sprites/shield_anim/sa-17.png", 0.08f},
                      {"data/sprites/shield_anim/sa-18.png", 0.08f},
                      {"data/sprites/shield_anim/sa-19.png", 0.08f},
                      {"data/sprites/shield_anim/sa-20.png", 0.08f},
                      {"data/sprites/shield_anim/sa-21.png", 0.08f},
                      {"data/sprites/shield_anim/sa-22.png", 0.08f},
                      {"data/sprites/shield_anim/sa-23.png", 0.08f},
                      {"data/sprites/shield_anim/sa-24.png", 0.08f}
              });

    Animation& explosion_anim = am.getAnimations().addItem();
    resources_.explosion_anim = explosion_anim.getId();
    explosion_anim.init({
                             {"data/sprites/explosion/ex-1.png", 0.06f},
                             {"data/sprites/explosion/ex-2.png", 0.06f},
                             {"data/sprites/explosion/ex-3.png", 0.12f},
                             {"data/sprites/explosion/ex-4.png", 0.12f},
                             {"data/sprites/explosion/ex-5.png", 0.12f},
                             {"data/sprites/explosion/ex-6.png", 0.12f},
                             {"data/sprites/explosion/ex-7.png", 0.06f},
                             {"data/sprites/explosion/ex-8.png", 0.06f}
                     });
}


GameEntity&Player::create(uint32_t owner_id) {
    MyGame& g = MyGame::get();
    GameEntity& ent = g.getSysEnt().getEntityManager().addItem();
    Player & me = ent.set<Player>(owner_id);
    me.my_id_ = ent.getId();

    CircleRenderable& body = me.getRenderables().add<CircleRenderable>(g, INITIAL_BODY_RADIUS);
    body.setLayerId(lPlayers);

    SpriteRenderable& arrow = me.getRenderables().add<SpriteRenderable>(g);
    arrow.setImageRegion(*resources_.arrow);
    arrow.setLayerId(lPlayers);
    arrow.getLocalTransform().setPosition({INITIAL_BODY_RADIUS-5, 0});

    RectRenderable& laser = me.getRenderables().add<RectRenderable>(g);
    laser.setColor(1, 0, 0, 1);
    laser.setSize(Vec2(MAX_LASER_LEN, 5));
    laser.getLocalTransform().setPosition(Vec2(INITIAL_BODY_RADIUS-1, 0));
    laser.setOffset(Vec2(0, -0.5f));
    laser.setLayerId(lPlayers);
    laser.setVisible(false);

    SpriteRenderable& shield = me.getRenderables().add<SpriteRenderable>(g);
    shield.setLayerId(lShield);
    shield.setAnimation(resources_.shield_anim);
    shield.setEndAction(SpriteRenderable::eaWrapAround);
    shield.setVisible(false);
    shield.getLocalTransform().setScale(Vec2(SHIELD_INTERNAL_SCALE, SHIELD_INTERNAL_SCALE));

    SpriteRenderable& explosion = me.getRenderables().add<SpriteRenderable>(g);
    explosion.setLayerId(lShield);
    explosion.setAnimation(resources_.explosion_anim);
    explosion.setPaused(true);
    explosion.getLocalTransform().setScale(Vec2(EXPLOSION_INTERNAL_SCALE, EXPLOSION_INTERNAL_SCALE));
    explosion.setVisible(false);

    return ent;
}


void Player::updateOnServer(InputStatePacket& client_input) {
    MyGame& g = MyGame::get();

    GameWorld& gw = g.getGameWorld();
    float dist_from_center = getTransform().getPosition().getLen();
    if (dist_from_center > gw.getWorldRadius() && !exploding_) {
        beginExploding();
        return;
    }
    if (exploding_) {
        if (getExplosion().isLastFrame())
            kill();
        return;
    }

    Window& w = g.getModule<Window>();

    Vec2 l_speed;
    Angle a_speed;

    Vec2 mouse_pos = client_input.getMousePos();
    Vec2 mouse_local = Mat3::invert(getTransform().getMatrix()) * w.getViewPort().viewToWorld(mouse_pos);
    Vec2 mouse_dir = normalize(mouse_local);
    float dist_to_mouse = mouse_local.getLen();
    float dist_out = dist_to_mouse - getBody().getOuterRadius();
    Vec2 move_dir(0, 0);
    if (dist_out > 0.0f)
        move_dir = getForwardDir();
    l_speed = move_dir * MOVEMENT_SPEED / getMassScaleFactor();

    float rot_dist = mouse_dir.getAngle().getRads();
    int rot_dir = sgn(rot_dist);
    rot_dist = (float) fabs(rot_dist);
    float rot_per_frame = ROTATION_SPEED / 60.0f;
    float rot_speed = (rot_per_frame > rot_dist) ? rot_dist * 60.0f : ROTATION_SPEED;

    a_speed = rot_dir * rot_speed;

    getLaser().setVisible(client_input.isLeftMouseDown());
    getShield().setVisible(client_input.isRightMouseDown());
    updateEnergy();

    fast_vector<GameEntity*> players;
    g.getAll<Player>(players);

    Circle body_bound = getBodyBound();
    Ray laser_ray = getLaserRay();

    laser_target_.target_id = VersionedIndex::Invalid();
    laser_target_.overlap_info = OverlapInfo();
    for (uint32_t i=0; i<players.size(); ++i) {
        if (players[i]->getId() == my_id_)
            continue;
        Player& p = players[i]->get<Player>();
        OverlapInfo body_oi;
        // body-body collisions
        if (body_bound.overlaps(p.getBodyBound(), body_oi)) {
            float mass_coeff = p.getMass()/(p.getMass()+getMass());
            Vec2 my_push_back = body_oi.getDirOut()*body_oi.getDepth()*mass_coeff;
            l_speed += my_push_back*60.f;
        }

        if (isShooting()) {
            // did i hit player?
            OverlapInfo laser_oi;
            Circle target_bound;
            if (p.isShieldOn() && !body_bound.overlaps(p.getShieldBound())) {
                target_bound = p.getShieldBound();
            }
            else {
                target_bound = p.getBodyBound();
            }

            if (!p.isExploding() && laser_ray.overlaps(target_bound, laser_oi)) {
                if (laser_oi.getDirOut() == laser_ray.getDir()) {
                    // seems i like am inside target body this must be rounding error -> fix
                    laser_oi.setDepth(5000);
                }
                if (laser_oi.getDepth() > laser_target_.overlap_info.getDepth()) {
                    laser_target_.overlap_info = laser_oi;
                    laser_target_.target_id = players[i]->getId();
                }
            }
        }
        float laser_len = MAX_LASER_LEN-laser_target_.overlap_info.getDepth();
        getLaser().setSize({laser_len, 5});

        if (p.isShooting()) {
            // am i hit by player laser ?
            Player::Target t = p.getLaserTarget();
            if (t.target_id == my_id_) {
                if (!isShieldOn() || p.getBodyBound().overlaps(getShieldBound())) {
                    // laser push
                    float mass_coeff = p.getMass()/getMass();

                    const Vec2& shooter_pos = p.getTransform().getPosition();
                    Vec2 me_to_shooter = shooter_pos - getTransform().getPosition();
                    Vec2 hit_to_shooter =  shooter_pos - t.overlap_info.getIntersection(0);
                    float mts_len = me_to_shooter.getLen();
                    float d = dot(me_to_shooter, hit_to_shooter) / mts_len;
                    d = mts_len-d;

                    float push_coeff = d/body_bound.getRadius();
                    Vec2 push_dir = normalize(getTransform().getPosition()-t.overlap_info.getIntersection(0));

                    l_speed += push_dir*push_coeff*LASER_PUSH_FORCE*mass_coeff;

                    float rot_coeff = 1- push_coeff;
                    float rot_dir = sgn(cross(me_to_shooter, hit_to_shooter));
                    a_speed -= rot_dir*rot_coeff*LASER_ROT_FORCE*mass_coeff;
                }
            }
        }
    }

    // mass eating
    for (uint32_t i=0; i<gw.getMassesCount(); ++i) {
        Circle mass_bound = gw.getMassBound(i);
        if (body_bound.overlaps(mass_bound)) {
            gw.removeMass(i);
            --i;
            setMass(mass_+MASS_INCREMENT);
        }
    }

    getSpeed().setLinearSpeed(l_speed);
    getSpeed().setAngularSpeed(a_speed);
}

void Player::updateOnClient(PlayerPacket& player_packet) {

    if (exploding_) {
        if (getExplosion().isLastFrame())
            kill();
        return;
    }
    else if (player_packet.getFlags()[2]) {
        beginExploding();
        return;
    }

    //Vec2 server_pos(player_packet.pos_x, player_packet.pos_y);
    //const Transform& t = getTransform();
    //std::cout << "diff from server pos: " << server_pos - t.getPosition() << std::endl;
    //setPosition(Vec2(player_packet.pos_x, player_packet.pos_y));
    //setRotation(player_packet.rot);
    setMass(player_packet.mass);
    energy_ = player_packet.energy;

    getLaser().setVisible(player_packet.getFlags()[0]);
    getShield().setVisible(player_packet.getFlags()[1]);

    if (isShooting()) {
        getLaser().setSize({player_packet.laser_len, 5});
    }

    getSpeed().setLinearSpeed(Vec2(player_packet.l_speed_x, player_packet.l_speed_y));
    getSpeed().setAngularSpeed(Angle(player_packet.a_speed));
}

Circle Player::getBodyBound() {
    float size_scale = getMassScaleFactor();
    return Circle(getTransform().getPosition(), INITIAL_BODY_RADIUS*size_scale);
}

Circle Player::getShieldBound() {
    float size_scale = getMassScaleFactor();
    return Circle(getTransform().getPosition(), getShield().getSize().getX()/2);
}

float Player::getMassScaleFactor() {
    return mass_/INITIAL_MASS;
}

bool Player::isShieldOn() {
    return getShield().getVisible();
}

float Player::getMaxEnergy() {
    return MAX_ENERGY;
}


CircleRenderable& Player::getBody() {
    return *(CircleRenderable*)getRenderables().get(0);
}

SpriteRenderable& Player::getArrow() {
    return *(SpriteRenderable*)getRenderables().get(1);
}

RectRenderable& Player::getLaser() {
    return *(RectRenderable*)getRenderables().get(2);
}

SpriteRenderable& Player::getShield() {
    return *(SpriteRenderable*)getRenderables().get(3);
}

SpriteRenderable& Player::getExplosion() {
    return *(SpriteRenderable*)getRenderables().get(4);
}

bool Player::isShooting() {
    return getLaser().getVisible();
}

Ray Player::getLaserRay() {
    RectRenderable& rr = getLaser();
    Vec2 pos = getTransform().getMatrix()*rr.getLocalTransform().getMatrix()*Vec2(0,0);
    Vec2 dir = Vec2(1.0f, 0.0f).rotate(getTransform().getRotation());
    return Ray(pos, dir, MAX_LASER_LEN);
}

Vec2 Player::getForwardDir() {
    return Vec2(1, 0).rotate(getTransform().getRotation());
}

void Player::setMass(float mass) {
    mass_ = mass;
    float size_scale = getMassScaleFactor();
    float new_radius = INITIAL_BODY_RADIUS*size_scale;
    getBody().setOuterRadius(new_radius);
    getArrow().getLocalTransform().setPosition({new_radius-5, 0});
    getLaser().getLocalTransform().setPosition({new_radius-1, 0});
    getShield().getLocalTransform().setScale(Vec2(size_scale*SHIELD_INTERNAL_SCALE, size_scale*SHIELD_INTERNAL_SCALE));
    getExplosion().getLocalTransform().setScale(Vec2(size_scale*EXPLOSION_INTERNAL_SCALE, size_scale*EXPLOSION_INTERNAL_SCALE));
}

void Player::beginExploding() {
    exploding_ = true;
    for (uint32_t i=0; i<4; ++i) {
        ((Renderable*)getRenderables().get(i))->setVisible(false);
    }
    getExplosion().setVisible(true);
    getExplosion().setPaused(false);
}

void Player::updateEnergy() {
    if (isShooting())
        energy_ -= LASER_DRAIN;
    if (isShieldOn())
        energy_ -= SHIELD_DRAIN;
    if (energy_<0) {
        energy_ = 0;
        getLaser().setVisible(false);
        getShield().setVisible(false);
    }
    if (!isShooting() && !isShieldOn()) {
        energy_ += ENERGY_RECHARGE;
        if (energy_ > MAX_ENERGY)
            energy_ = MAX_ENERGY;
    }
}