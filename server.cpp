//
// Created by jakub on 13.12.17.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctime>
#include <error.h>
#include <cerrno>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <thread>
#include <random>
#include "GameStructs.h"

using namespace std;

typedef struct PlayerInfo {
    string name;
    sockaddr_in address{};
    PlayerState plState{};
} PlayerInfo;

int playersNo = 0;
int moveSpeed = 3;
int serverFd;

int maxBullets = 30;
int mapSizeX = 800;
int mapSizeY = 600;

AllState gameData;
PlayerInfo players[16];
PlayerState states[16];
vector<Bullet> bullets;

void movePlayer(PlayerInput *input, PlayerInfo &pl) {
    if (input->left) {
        if (pl.plState.x - moveSpeed < 0)
            pl.plState.x = 0;
        else
            pl.plState.x -= moveSpeed;
    }
    if (input->right) {
        if (pl.plState.x + moveSpeed > mapSizeX)
            pl.plState.x = mapSizeX;
        else
            pl.plState.x += moveSpeed;
    }
    if (input->up) {
        if (pl.plState.y - moveSpeed < 0)
            pl.plState.y = 0;
        else
            pl.plState.y -= moveSpeed;
    }
    if (input->down) {
        if (pl.plState.y + moveSpeed > mapSizeY)
            pl.plState.y = mapSizeY;
        else
            pl.plState.y += moveSpeed;
    }
}

void receiving() {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t slen = sizeof(clientAddr);
        if (playersNo < 1)
            break;
        char buffer[2048];
        memset(buffer, 0, sizeof(buffer));
        bool noInput = false;
        ssize_t recsize;

        recsize = recvfrom(serverFd, buffer, sizeof buffer, 0, (sockaddr *) &clientAddr, &slen);
        if (recsize < 0) {
            noInput = true;
        }
        if (!noInput) {
            auto *input = (PlayerInput *) buffer;
            //cout << "data from " << input->name << endl;
            for (auto &pl : players) {
                if (strcmp(input->name, pl.name.c_str()) == 0) {
                    movePlayer(input, pl);
                    if (input->shoot && bullets.size() < maxBullets) {
                        bullets.push_back(Bullet{(float)pl.plState.x, (float)pl.plState.y, input->xDir, input->yDir});
                    }
                }
            }
        }
    }
}

int main() {
    random_device rd;
    mt19937 mt(rd());
    uniform_int_distribution<int> idistx(0, 800);
    uniform_int_distribution<int> idisty(0, 600);

    int timeToConnect = 20;

    serverFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverFd == -1) {
        error(1, errno, "socket");
        return 1;
    }
    sockaddr_in serverAddr{.sin_family=AF_INET, .sin_port=htons(50000), .sin_addr={INADDR_ANY}};

    const int one = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    int res = bind(serverFd, (sockaddr *) &serverAddr, sizeof(serverAddr));
    if (res) {
        error(1, errno, "bind");
        close(serverFd);
        return 1;
    }
    sockaddr_in clientAddr{};
    socklen_t slen = sizeof(clientAddr);
    ssize_t recsize;

    cout << "Server up on " << inet_ntoa(serverAddr.sin_addr) << " : " << ntohs(serverAddr.sin_port) << endl;

    struct timeval read_timeout{};
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10000;
    setsockopt(serverFd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    time_t start = time(nullptr);

    for (;;) {
        char buffer[255];
        memset(buffer, 0, sizeof(buffer));

        recsize = recvfrom(serverFd, buffer, sizeof buffer, 0, (sockaddr *) &clientAddr, &slen);
        if (recsize < 0) {
            if (time(nullptr) - start > timeToConnect)
                break;
            else
                continue;
        }

        bool nameInUse = false;

        for (int i = 0; i < playersNo; i++) {
            if (strcmp(players[i].name.c_str(), buffer) == 0) {
                cout << players[i].name.c_str() << "  =  " << buffer << endl;
                nameInUse = true;
                string returnMessage = "nameinuse";
                sendto(serverFd, returnMessage.c_str(), returnMessage.size(), 0, (sockaddr *) &clientAddr, slen);
            }
        }

        if (!nameInUse) {
            players[playersNo].name = buffer;
            players[playersNo].address = clientAddr;
            playersNo++;

            string returnMessage = "welcome";
            sendto(serverFd, returnMessage.c_str(), returnMessage.size(), 0, (sockaddr *) &clientAddr, slen);
        }

        for (int i = 0; i < playersNo; i++) {
            cout << players[i].name.c_str() << "  -  " << inet_ntoa(players[i].address.sin_addr)
                 << " : " << htons(players[i].address.sin_port) << endl;
        }

        if (time(nullptr) - start > timeToConnect)
            break;
    }

    cout << endl << "Game starts now!" << endl << endl;
    string letsgomsg = "lets go";

    for (int i = 0; i < playersNo; i++) {
        states[i].x = idistx(mt);
        states[i].y = idisty(mt);
        strcpy(states[i].name, players[i].name.c_str());
        players[i].plState = states[i];
    }

    for (int i = 0; i < playersNo; i++) {
        cout << players[i].name << " : " << players[i].plState.x << " - " << players[i].plState.y << endl;
        sendto(serverFd, letsgomsg.c_str(), letsgomsg.size(), 0, (sockaddr *) &players[i].address, slen);
    }

    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 50000; // im wieksze tym mniejszy lag ale stutter przy obracaniu
    setsockopt(serverFd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    thread t(receiving);

    while (true) {
        if (playersNo < 1)
            break;

        char buffer[2048];
        memset(buffer, 0, sizeof(buffer));

        gameData.numberOfPlayers = playersNo;
        for (int i = 0; i < playersNo; i++) {
            gameData.players[i] = players[i].plState;
        }

        gameData.numberOfBullets = static_cast<int>(bullets.size());

        auto i = bullets.begin();
        while (i != bullets.end()){
            i->xPos +=  i->xDir;
            i->yPos += i->yDir;
            if (i->xPos < 0 || i->xPos > mapSizeX || i->yPos < 0 || i->yPos > mapSizeY)
                i = bullets.erase(i);
            else
                i++;
        }

        copy(bullets.begin(), bullets.end(), gameData.bullets);

        for (auto pl : players) {
            sendto(serverFd, &gameData, sizeof(gameData), 0, (sockaddr *) &pl.address, slen);
        }

        usleep(16667); // 60 Hz
    }
    return 0;
}
