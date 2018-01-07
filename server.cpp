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

using namespace std;

typedef struct PlayerInput{bool left; bool right; bool up; bool down; char name[20]; } PlayerInput;
typedef struct GameState{int x; int y; } GameState; // bedzie usuniete

typedef struct PlayerState{int x; int y; char name[20]; } PlayerState;
typedef struct PlayerInfo{string name; sockaddr_in address{}; PlayerState plState{}; } PlayerInfo;
typedef struct AllState{int numberOfPlayers{}; PlayerState players[16]; } AllState;

int playersNo = 0;

AllState gameData;
PlayerInfo players[16];
PlayerState states[16];

void hello() {
    cout << "this thread is working" << endl;
}

int main(){
    random_device rd;
    mt19937 mt(rd());
    uniform_int_distribution<int> idistx(0, 800);
    uniform_int_distribution<int> idisty(0, 600);
    int timeToConnect = 20;
    int x = 128, y = 128;
    int moveSpeed = 3;
    int serverFd = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverFd == -1) {
        error(1, errno, "socket");
        return 1;
    }
    sockaddr_in serverAddr{.sin_family=AF_INET, .sin_port=htons(50000), .sin_addr={INADDR_ANY}};

    const int one = 1;
    setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    int res = bind(serverFd, (sockaddr*) &serverAddr, sizeof(serverAddr));
    if(res) {
        error(1, errno, "bind");
        close(serverFd);
        return 1;
    }
    sockaddr_in clientAddr{};
    socklen_t slen = sizeof(clientAddr);
    ssize_t recsize;

    std::thread t(hello);

    t.join();

    cout << "Server up on " << inet_ntoa(serverAddr.sin_addr) << " : " << ntohs(serverAddr.sin_port) << endl;

    // idk

    struct timeval read_timeout{};
    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 10000;
    setsockopt(serverFd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    clock_t begin = clock();

    for(;;){
        char buffer[255];
        memset(buffer, 0, sizeof(buffer));

        recsize = recvfrom(serverFd, buffer, sizeof buffer, 0, (sockaddr*) &clientAddr, &slen);
        if (recsize < 0) {
            //error(1, errno, "receive");
            //break;
            if ((double(clock() - begin) / CLOCKS_PER_SEC*500) > timeToConnect) // zle
                break;
            else
                continue;
        }

        bool nameInUse = false;

        for (int i = 0; i < playersNo; i++)
        {
            if (strcmp(players[i].name.c_str(), buffer) == 0)
            {
                cout << players[i].name.c_str() << "  =  " << buffer << endl;
                nameInUse = true;
                string returnMessage = "nameinuse";
                sendto(serverFd, returnMessage.c_str(), returnMessage.size(), 0, (sockaddr *) &clientAddr, slen);
            }
        }

        if (!nameInUse){
            players[playersNo].name = buffer;
            players[playersNo].address = clientAddr;
            playersNo++;

            string returnMessage = "welcome";
            sendto(serverFd, returnMessage.c_str(), returnMessage.size(), 0, (sockaddr *) &clientAddr, slen);
        }

        for (int i = 0; i < playersNo; i++)
        {
                cout << players[i].name.c_str() << "  -  " << inet_ntoa(players[i].address.sin_addr)
                     << " : " << htons(players[i].address.sin_port) << endl;
        }

        //cout << to_string(double(clock() - begin) / CLOCKS_PER_SEC*1000) << endl;
        if ((double(clock() - begin) / CLOCKS_PER_SEC*500) > timeToConnect)
            break;
    }

    cout << endl << "Game starts now! time: " << to_string(double(clock() - begin) / CLOCKS_PER_SEC*1000) << endl << endl;
    string letsgomsg = "lets go";

    for (int i = 0; i < playersNo; i++){
        states[i].x = idistx(rd);
        states[i].y = idisty(rd);
        strcpy(states[i].name, players[i].name.c_str());
        players[i].plState = states[i];
    }

    for (int i = 0; i < playersNo; i++){
        cout << players[i].name << " : " << players[i].plState.x << " - " << players[i].plState.y << endl;
        sendto(serverFd, letsgomsg.c_str(), letsgomsg.size(), 0, (sockaddr *) &players[i].address, slen);
    }

    read_timeout.tv_sec = 0;
    read_timeout.tv_usec = 100000;
    setsockopt(serverFd, SOL_SOCKET, SO_RCVTIMEO, &read_timeout, sizeof read_timeout);

    bool noInput = false;

    while(true){
        if (playersNo < 1)
            break;
        char buffer[255];
        memset(buffer, 0, sizeof(buffer));

        recsize = recvfrom(serverFd, buffer, sizeof buffer, 0, (sockaddr*) &clientAddr, &slen);
        if (recsize < 0) {
            //error(1, errno, "receive");
            //break;
            //continue;
            noInput = true;
            cout << "no input" << endl;
        }
        if (!noInput) {
            auto *input = (PlayerInput *) buffer;
            cout << "data from " << input->name << endl;
            for (auto &pl : players) {
                if (strcmp(input->name,pl.name.c_str()) == 0) {
                    if (input->left) pl.plState.x -= moveSpeed;
                    if (input->right) pl.plState.x += moveSpeed;
                    if (input->up) pl.plState.y -= moveSpeed;
                    if (input->down) pl.plState.y += moveSpeed;
                }
            }
        }
        //TODO * time elapsed

        gameData.numberOfPlayers = playersNo;
        for (int i = 0; i < playersNo; i++){
            gameData.players[i] = players[i].plState;
        }

        for (auto pl : players){
//            cout << "Sending: " + to_string(state.x) + " " + to_string(state.y) << endl;
            sendto(serverFd, &gameData, sizeof(gameData), 0, (sockaddr *) &pl.address, slen);
        }

        noInput = false;
//        auto state = GameState{x, y};
//        cout << "Sending: " + to_string(state.x) + " " + to_string(state.y) << endl;
//        sendto(serverFd, &state, sizeof(state), 0, (sockaddr *) &clientAddr, slen);
    }
    return 0;
}
