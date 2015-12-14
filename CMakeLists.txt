cmake_minimum_required(VERSION 3.2)
project(LD34)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -DGLM_FORCE_RADIANS -std=gnu++0x")

#base
include_directories(/home/grynca/DEV/gamedev/base/src)
#sysent
include_directories(/home/grynca/DEV/gamedev/sysent/include/)
#maths
include_directories(/home/grynca/DEV/gamedev/maths/src/)
#SAP
include_directories(/home/grynca/DEV/gamedev/SAP/include/)
#assets
include_directories(/home/grynca/DEV/gamedev/assets/include/)
#geng
include_directories(/home/grynca/DEV/gamedev/geng/include/)
link_directories(/home/grynca/DEV/gamedev/geng/build/Debug/)
#freetype
include_directories(/usr/include/freetype2)

set(LIBS "-lgeng -lGLEW -lGL -lGLU -lfreetype -lSDL2 -lSDL2_image -lSDL2_net")


set(SOURCE_FILES src/main.cpp
        src/incl.h
        src/MyGame.h
        src/entities/Player.h
        src/MyGame.inl
        src/entities/Player.cpp
        src/Layers.h
        src/MyAssets.h
        src/entities/GameWorld.h
        src/entities/GameWorld.cpp
        src/Network.h
        src/Network.cpp
        src/packets.h
        src/packets.cpp
)
add_executable(LD34 ${SOURCE_FILES})
target_link_libraries(LD34 ${LIBS})