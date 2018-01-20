//
// Created by jakub on 19.01.18.
//

#ifndef GAMESK2_GAMESTRUCTS_H
#define GAMESK2_GAMESTRUCTS_H

#define MAX_BULLETS 50
#define MAX_PLAYERS 16

typedef struct PlayerInput {
    bool left;
    bool right;
    bool up;
    bool down;
    bool shoot;
    char name[20];
    float xDir;
    float yDir;
} PlayerInput;

typedef struct Bullet {
    float xPos;
    float yPos;
    float xDir;
    float yDir;
} Bullet;

typedef struct PlayerState {
    int x;
    int y;
    char name[20];
    bool alive;
} PlayerState;

typedef struct AllState {
    int numberOfPlayers{};
    PlayerState players[MAX_PLAYERS]{};
    int numberOfBullets{};
    Bullet bullets[MAX_BULLETS]{};
} AllState;

#endif //GAMESK2_GAMESTRUCTS_H
