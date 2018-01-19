//
// Created by jakub on 19.01.18.
//

#ifndef GAMESK2_GAMESTRUCTS_H
#define GAMESK2_GAMESTRUCTS_H

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
} PlayerState;

typedef struct AllState {
    int numberOfPlayers{};
    PlayerState players[16]{};
    int numberOfBullets{};
    Bullet bullets[30]{};
} AllState;

#endif //GAMESK2_GAMESTRUCTS_H
