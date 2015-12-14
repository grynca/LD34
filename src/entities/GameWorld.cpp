#include "../incl.h"

#define WORLD_RADIUS 350
#define MASS_APPEAR_INTERVAL 100
#define MASS_CIRCLE_RADIUS 5
#define MASS_SPAWN_TRIES 10
#define MASS_MIN_DISTANCE 80

GameWorld::GameWorld()
 : to_next_mass_(MASS_APPEAR_INTERVAL)
{}


void GameWorld::initResources() {

}

GameEntity& GameWorld::create() {
    MyGame& g = MyGame::get();

    GameEntity& ent = g.getSysEnt().getEntityManager().addItem();
    GameWorld& me = ent.set<GameWorld>();

    CircleRenderable& land = me.getRenderables().add<CircleRenderable>(g, WORLD_RADIUS);
    land.setLayerId(lBackground);
    land.setColor(0.5, 0.5, 0.5);

    ViewPort& vp = g.getModule<Window>().getViewPort();

    TextRenderable& energy_label = me.getRenderables().add<TextRenderable>().init(g);
    energy_label.setTextureUnit(1);
    energy_label.setLayerId(10);
    energy_label.getTextSetter().setFont(0).setFontSize(30).setText("Energy: ");
    energy_label.setCoordFrame(cfScreen);
    energy_label.getLocalTransform().setPosition(Vec2(vp.getBaseSize().getX()-330, vp.getBaseSize().getY()-15));

    RectRenderable& energybar = me.getRenderables().add<RectRenderable>(g);
    energybar.setColor(1.0, 0.0, 0.0);
    energybar.setLayerId(lHUD);
    energybar.setCoordFrame(cfScreen);
    energybar.setOffset(Vec2(0.0f, -0.5f));
    energybar.getLocalTransform().setPosition({vp.getBaseSize().getX()-210, vp.getBaseSize().getY()-25});
    energybar.setSize(Vec2(200, 30));
    return ent;
}


void GameWorld::updateOnServer() {
    MyGame& g = MyGame::get();

    --to_next_mass_;
    if (!to_next_mass_) {
        if (getMassesCount() < MAX_MASSES)
            spawnMass_(g);
        to_next_mass_ = MASS_APPEAR_INTERVAL;
    }

    Player* p = g.getMyPlayer();
    float energy = Player::getMaxEnergy();
    if (p) {
        energy = p->getEnergy()/Player::getMaxEnergy() * 200;
    }
    getEnergyBar_().setSize(Vec2(energy, 30));
}

void GameWorld::updateOnClient(GameWorldPacket& gwp) {
    MyGame& g = MyGame::get();

    clearMasses_();
    for (uint32_t i=0; i<gwp.masses_count; ++i) {
        addMass_(Vec2(gwp.masses_x[i], gwp.masses_y[i]));
    }

    Player* p = g.getMyPlayer();
    float energy = Player::getMaxEnergy();
    if (p) {
        energy = p->getEnergy()/Player::getMaxEnergy() * 200;
    }
    getEnergyBar_().setSize(Vec2(energy, 30));
}

RectRenderable& GameWorld::getEnergyBar_() {
    return *(RectRenderable*)getRenderables().get(2);
}


void GameWorld::spawnMass_(MyGame& g) {
    Vec2 good_pos;
    for (uint32_t tid=0; tid<MASS_SPAWN_TRIES; ++tid) {
        // spawn mass circle
        Angle t(randFloat()*2*Angle::Pi);
        float u = randFloat()+randFloat();
        float r = (u>1)?2-u:u;
        r *= WORLD_RADIUS*0.8;
        Vec2 pos = Vec2(r*t.getCos(), r*t.getSin());

        // check if not close to other renderables or players
        Circle bound(pos, MASS_MIN_DISTANCE);
        uint32_t i;
        for (i=0; i<getMassesCount(); ++i) {
            if (bound.overlaps(getMassBound(i)))
                break;
        }
        if (i!=getMassesCount())
            continue;

        fast_vector<GameEntity*> players;
        g.getAll<Player>(players);
        for (i=0; i<players.size(); ++i) {
            Player& p = players[i]->get<Player>();
            if (bound.overlaps(p.getBodyBound()))
                break;
        }
        if (i!=players.size())
            continue;

        good_pos = pos;
        break;
    }

    if (!good_pos.isZero()) {
        addMass_(good_pos);
    }
}

void GameWorld::addMass_(const Vec2& pos) {
    MyGame& g = MyGame::get();
    CircleRenderable& mc = getRenderables().add<CircleRenderable>(g, MASS_CIRCLE_RADIUS);
    mc.setLayerId(lMassCircles);
    mc.getLocalTransform().setPosition(pos);
}

void GameWorld::clearMasses_() {
    while (getRenderables().getSize()>3) {
        getRenderables().remove(getRenderables().getSize()-1);
    }
}

uint32_t GameWorld::getMassesCount() {
    return getRenderables().getSize() - 3;
}

Circle GameWorld::getMassBound(uint32_t id) {
    id += 3;
    CircleRenderable& cr = *(CircleRenderable*)(getRenderables().get(id));
    return Circle(cr.getLocalTransform().getPosition(), MASS_CIRCLE_RADIUS);
}

void GameWorld::removeMass(uint32_t id) {
    getRenderables().remove(id +3);
}

float GameWorld::getWorldRadius() {
    return WORLD_RADIUS;
}