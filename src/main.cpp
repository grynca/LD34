#include <iostream>

#include "incl.h"
#include "MyGame.h"

using namespace std;

int main(int argc, char *argv[]) {
    MyGame& game = MyGame::create();

    std::string servername = "localhost";
    uint16_t port = 1337;
    if (argc > 1)
        servername = argv[1];
    else {
        game.startServer(port);
    }
    game.connectToServer(servername, port);

    game.start();
    return 0;
}