#include "core/Game.hpp"

int main() {
    sc::Game game;
    sc::gameInit(game, 1280, 720, "sidescroler");
    sc::gameRun(game);
    sc::gameShutdown(game);
    return 0;
}
