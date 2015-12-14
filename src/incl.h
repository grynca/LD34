#ifndef INCL_H
#define INCL_H

#include "geng.h"
#include "assets.h"
using namespace grynca;
#include "Layers.h"
#include "MyAssets.h"
#include "Network.h"
#include "packets.h"

class MyGame;
class ClientGame;
class GameWorld;
class Player;

typedef grynca::TypesPack<
        GameWorld,
        Player
> EntityTypes;
typedef grynca::TypesPack<
        grynca::StartTickSystem<MyGame>,
        grynca::MovementSystem<MyGame>
        //grynca::CollisionSystem<MyGame>
> UpdateSystemTypes;
typedef grynca::TypesPack<
        grynca::RenderSystem<MyGame>
> RenderystemTypes;

DEFINE_ENUM_E(EntityRoles, GengEntityRoles,
              erDummy, erDummy2
);

typedef grynca::Entity<EntityTypes> GameEntity;

typedef grynca::ES<EntityTypes, UpdateSystemTypes, RenderystemTypes, EntityRoles> MyEntSys;

#include "entities/GameWorld.h"
#include "entities/Player.h"

#include "MyGame.h"

#endif //INCL_H
