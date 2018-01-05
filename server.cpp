//
// Created by jakub on 13.12.17.
//

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <error.h>
#include <cerrno>
#include <unistd.h>
#include <iostream>
#include <cstring>
#include <arpa/inet.h>

using namespace std;

typedef struct PlayerInput{bool left; bool right; bool up; bool down; bool shoot; } PlayerInput;
typedef struct GameState{int x; int y; } GameState;

int main(){
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

    cout << "Server up on " << inet_ntoa(serverAddr.sin_addr) << " : " << ntohs(serverAddr.sin_port) << endl;
    while(true){
        char buffer[255];
        memset(buffer, 0, sizeof(buffer));

        recsize = recvfrom(serverFd, buffer, sizeof buffer, 0, (sockaddr*) &clientAddr, &slen);
        if (recsize < 0) {
            error(1, errno, "receive");
            break;
        }
        auto * input = (PlayerInput*) buffer;

        //TODO * time elapsed
        if (input->left) x -= moveSpeed;
        if (input->right) x += moveSpeed;
        if (input->up) y -= moveSpeed;
        if (input->down) y += moveSpeed;

        auto state = GameState{x, y};
        cout << "Sending: " + to_string(state.x) + " " + to_string(state.y) << endl;
        sendto(serverFd, &state, sizeof(state), 0, (sockaddr *) &clientAddr, slen);
    }
    return 0;
}
