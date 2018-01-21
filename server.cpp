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

int moveSpeed = 3;
int serverFd;

int mapSizeX = 800;
int mapSizeY = 600;

AllState gameData;
vector<PlayerInfo> players;
vector<Bullet> bullets;

void movePlayer(PlayerInput *input, PlayerInfo &pl) {
    pl.plState.yDir = 0;
    pl.plState.xDir = 0;
    if (input->left) {
        if (pl.plState.x - moveSpeed < 0)
            pl.plState.x = 0;
        else {
            pl.plState.x -= moveSpeed;
            pl.plState.xDir -= 1;
        }
    }
    if (input->right) {
        if (pl.plState.x + moveSpeed > mapSizeX)
            pl.plState.x = mapSizeX;
        else {
            pl.plState.x += moveSpeed;
            pl.plState.xDir += 1;
        }
    }
    if (input->up) {
        if (pl.plState.y - moveSpeed < 0)
            pl.plState.y = 0;
        else {
            pl.plState.y -= moveSpeed;
            pl.plState.yDir -= 1;
        }
    }
    if (input->down) {
        if (pl.plState.y + moveSpeed > mapSizeY)
            pl.plState.y = mapSizeY;
        else {
            pl.plState.y += moveSpeed;
            pl.plState.yDir += 1;
        }
    }
}

void receiving() {
    while (true) {
        sockaddr_in clientAddr{};
        socklen_t slen = sizeof(clientAddr);
        if (players.empty()) break;
        char buffer[2048];
        memset(buffer, 0, sizeof(buffer));
        bool noInput = false;
        ssize_t recsize;

        try {
            recsize = recvfrom(serverFd, buffer, sizeof buffer, 0, (sockaddr *) &clientAddr, &slen);
            if (recsize < 0) {
                noInput = true;
            }
        } catch(const exception &e) {
            cerr << e.what() << '\n';
        }

        if (!noInput) {
            auto *input = (PlayerInput *) buffer;
            for (auto &pl : players) {
                if (strcmp(input->name, pl.name.c_str()) == 0) {
                    if (!pl.plState.alive) break;
                    movePlayer(input, pl);
                    if (input->shoot && bullets.size() < MAX_BULLETS) {
                        bullets.push_back(Bullet{(pl.plState.x + 50 * input->xDir),
                                                 (pl.plState.y + 50 * input->yDir),
                                                 input->xDir, input->yDir});
                    }
                    break;
                }
            }
        }
    }
}

void sending(socklen_t slen) {
    while(true){
        if (players.empty()) break;
        for (auto pl : players) {
            sendto(serverFd, &gameData, sizeof(gameData), 0, (sockaddr *) &pl.address, slen);
        }
        usleep(50000);
    }
}

int main() {
    random_device rd;
    mt19937 mt(rd());
    uniform_int_distribution<int> idistx(0, mapSizeX);
    uniform_int_distribution<int> idisty(0, mapSizeY);

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

        for (auto pl : players) {
            if (strcmp(pl.name.c_str(), buffer) == 0) {
                cout << pl.name.c_str() << "  =  " << buffer << endl;
                nameInUse = true;
                string returnMessage = "nameinuse";
                sendto(serverFd, returnMessage.c_str(), returnMessage.size(), 0, (sockaddr *) &clientAddr, slen);
                break;
            }
        }

        if (!nameInUse) {
            PlayerInfo inf{};
            inf.name = buffer;
            inf.address = clientAddr;
            players.push_back(inf);

            string returnMessage = "welcome";
            sendto(serverFd, returnMessage.c_str(), returnMessage.size(), 0, (sockaddr *) &clientAddr, slen);
        }

        for (auto pl : players) {
            cout << pl.name << "  -  " << inet_ntoa(pl.address.sin_addr)
                 << " : " << htons(pl.address.sin_port) << endl;
        }

        if (time(nullptr) - start > timeToConnect)
            break;
    }

    cout << endl << "Game starts now!" << endl << endl;
    string letsgomsg = "lets go";

    for (auto &pl : players) {
        PlayerState st{idistx(mt), idisty(mt)};
        st.alive = true;
        strcpy(st.name, pl.name.c_str());
        pl.plState = st;
    }

    for (auto pl : players) {
        cout << pl.name << " : " << pl.plState.x << " - " << pl.plState.y << endl;
        sendto(serverFd, letsgomsg.c_str(), letsgomsg.size(), 0, (sockaddr *) &pl.address, slen);
    }

    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 50000; // im wieksze tym mniejszy lag ale stutter przy obracaniu
    setsockopt(serverFd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    thread recthread(receiving);
    thread sendthread(sending, slen);

    while (true) {
        if (players.empty())
            break;

        //char buffer[2048];
        //memset(buffer, 0, sizeof(buffer));

        gameData.numberOfPlayers = (int)players.size();
        for (int i = 0; i < gameData.numberOfPlayers; i++) {
            gameData.players[i] = players[i].plState;
        }

        bool hit;

        auto i = bullets.begin();
        while (i != bullets.end()){
            hit = false;
            i->xPos +=  i->xDir;
            i->yPos += i->yDir;
            if (i->xPos < 0 || i->xPos > mapSizeX || i->yPos < 0 || i->yPos > mapSizeY) {
                i = bullets.erase(i);
                continue;
            }
            else {
                auto j = players.begin();
                while (j != players.end()) {
                    if (!j->plState.alive) {
                        j++;
                        continue;
                    }
                    if (max(abs(i->xPos - j->plState.x), abs(i->yPos - j->plState.y)) < 30) {
                        i = bullets.erase(i);
                        //j = players.erase(j);
                        j->plState.alive = false;
                        j->plState.xDir = 0;
                        j->plState.yDir = 0;
                        hit = true;
                        break;
                    }
                    else j++;
                }
            }
            if (!hit) i++;
        }

        gameData.numberOfBullets = static_cast<int>(bullets.size());
        copy(bullets.begin(), bullets.end(), gameData.bullets);

        usleep(16667); // 60 Hz
        //usleep(50000); // 20 Hz
        //usleep(150000);
    }
    recthread.join();
    sendthread.join();
    return 0;
}
