#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <thread>
#include <iostream>
#include <sstream>
#include <cstring>
#include <atomic>
#include "GameStructs.h"

using namespace std;

template<typename T>
sf::Vector2<T> normalize(const sf::Vector2<T> &source) {
    float length = sqrt((source.x * source.x) + (source.y * source.y));
    if (length != 0)
        return sf::Vector2<T>(source.x / length, source.y / length);
    else
        return source;
}

atomic<bool> gameStarted;

sf::UdpSocket socket;

string myID;

PlayerInput input = PlayerInput{};

sf::RenderWindow window;

int controls = 0;

// cos nie chce dzialac watkowo
//void sendInput(string name, sf::IpAddress recipient, unsigned short port){
/*void sendInput(){
    while(true) {
        if (controls == 0) {
            input.left = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
            input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
            input.up = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
            input.down = sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
            //input.shoot = sf::Mouse::isButtonPressed(sf::Mouse::Left);
        } else {
            input.left = sf::Keyboard::isKeyPressed(sf::Keyboard::A);
            input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::D);
            input.up = sf::Keyboard::isKeyPressed(sf::Keyboard::W);
            input.down = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
            //input.shoot = sf::Mouse::isButtonPressed(sf::Mouse::Right);
        }

        strcpy(input.name, myID.c_str());

        if (input.up || input.right || input.left || input.down) {

            if (socket.send(&input, sizeof input, recipient, port) != sf::Socket::Done) {
                std::cout << "send error" << std::endl;
                break;
            }
        }
        usleep(10000);
    }
}*/

void loadingScreen(const sf::Sprite &bg, sf::Sprite circle) {
    sf::Clock clock;
    while (window.isOpen() && !gameStarted) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        //circle.rotate(1);
        circle.setRotation(91 * clock.getElapsedTime().asSeconds());
        window.clear();
        window.draw(bg);
        window.draw(circle);
        window.display();
    }
}

int main() {
    cout << "Podaj nick" << endl;
    getline(cin, myID);
    cout << "Wybierz sterowanie: 0 - strzalki, else - wsad" << endl;
    cin >> controls;

    gameStarted = false;

    unsigned short startport = 54000;

    while (socket.bind(startport) != sf::Socket::Done) {
        std::cout << "cant bind to " << startport << std::endl;
        ++startport;
    }

    sf::IpAddress recipient = "127.0.0.1";
    unsigned short port = 50000;

    window.create(sf::VideoMode(800, 600), "Wow ale fajna gra jeszcze w takom nie grauem");
    window.setKeyRepeatEnabled(false);
    window.setVerticalSyncEnabled(true);

    // view setup
    sf::View view(sf::Vector2f(128, 128), sf::Vector2f(400, 300));

    sf::Texture loadingbg;
    if (!loadingbg.loadFromFile("resources/loadingbg.png")) {
        return 0;
    }
    sf::Texture circle;
    if (!circle.loadFromFile("resources/loadingcircle.png")) {
        return 0;
    }
    sf::Texture texture;
    if (!texture.loadFromFile("resources/Pacman 3.png")) {
        return 0;
    }
    sf::Texture map;
    if (!map.loadFromFile("resources/map.png")) {
        return 0;
    }
    sf::Texture bullet;
    if (!bullet.loadFromFile("resources/bullet.png")) {
        return 0;
    }

    sf::Sprite mapSprite;
    mapSprite.setTexture(map);

    sf::Sprite loadingSprite(loadingbg);
    sf::Sprite circleSprite(circle);
    circleSprite.setOrigin(50, 50);
    circleSprite.setPosition(400, 400);

    vector<sf::Sprite> bulletSprites(MAX_BULLETS, sf::Sprite(bullet));

//    float bulletInterval = 2.0f;
    texture.setSmooth(true);

    vector<sf::Sprite> sprites(16, sf::Sprite(texture));

    for (auto &sprite : sprites) {
        sprite.setPosition(1.0, 2.0);
        sprite.setOrigin(sf::Vector2f(128, 128));
        sprite.setScale(0.3f, 0.3f);
    }

    sf::Clock bulletTimer;

    // wejscie do gry
    if (socket.send(myID.c_str(), myID.size(), recipient, port) != sf::Socket::Done) {
        std::cout << "send error" << std::endl;
    }

    char data[100];
    memset(data, 0, sizeof(data));
    size_t received;
    sf::IpAddress sender;
    unsigned short remotePort;
    if (socket.receive(data, 100, received, sender, remotePort) != sf::Socket::Done) {
        std::cout << "receive error" << std::endl;
    }

    cout << "Message: " << data << endl;
    if (strcmp(data, "nameinuse") == 0) {
        cout << "This name is already in use. Bye bye." << endl;
        return -1;
    }

    cout << "Waiting for the game to start" << endl;

    thread loadingThread(loadingScreen, loadingSprite, circleSprite);

    if (socket.receive(data, 100, received, sender, remotePort) != sf::Socket::Done) {
        std::cout << "receive error" << std::endl;
    }

    gameStarted = true;
    loadingThread.join();

    strcpy(input.name, myID.c_str());

    // petla gry
    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        //sendInput(myID, recipient, port);

        if (controls == 0) {
            input.left = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
            input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
            input.up = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
            input.down = sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
            input.shoot = sf::Mouse::isButtonPressed(sf::Mouse::Left);
        } else {
            input.left = sf::Keyboard::isKeyPressed(sf::Keyboard::A);
            input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::D);
            input.up = sf::Keyboard::isKeyPressed(sf::Keyboard::W);
            input.down = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
            input.shoot = sf::Mouse::isButtonPressed(sf::Mouse::Right);
        }


        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f aimDirection =
                sf::Vector2f(mousePos) - sf::Vector2f(window.getSize().x / 2, window.getSize().y / 2);

        double radianRotation = atan2(aimDirection.x, -aimDirection.y) / M_PI * 180.0;

//        sf::Vector2f normalDir = normalize(aimDirection);

        if (input.up || input.right || input.left || input.down || input.shoot) {
            if (input.shoot) {
                sf::Vector2f normalDir = normalize(aimDirection);
                input.xDir = normalDir.x;
                input.yDir = normalDir.y;
            }
            if (socket.send(&input, sizeof input, recipient, port) != sf::Socket::Done) {
                std::cout << "send error" << std::endl;
            }
        }


        char gameData[2048];
        memset(gameData, 0, sizeof(gameData));
        size_t received1;
        sf::IpAddress sender1;
        unsigned short remotePort1;
        if (socket.receive(gameData, 2048, received1, sender1, remotePort1) != sf::Socket::Done) {
            std::cout << "receive error" << std::endl;
        }

        auto *state = (AllState *) gameData;

        int me = 0;

        for (int i = 0; i < state->numberOfPlayers; i++) {
            sprites[i].setPosition(state->players[i].x, state->players[i].y);
            if (strcmp(myID.c_str(), state->players[i].name) == 0)
                me = i;
        }

        for (int i = 0; i < state->numberOfBullets; i++) {
            bulletSprites[i].setPosition(state->bullets[i].xPos, state->bullets[i].yPos);
        }

        /*if (bulletTimer.getElapsedTime().asSeconds() > bulletInterval && input.shoot) {
            aim = normalize(aimDirection);
            bulletSprite.setPosition(sprites[me].getPosition());
            bulletUp = true;
            bulletTimer.restart();
        } else {
            bulletSprite.move(sf::Vector2f(aim.x * 2, aim.y * 2));
        }*/

        sprites[me].setRotation((float) radianRotation);
        view.setCenter(sprites[me].getPosition());
        window.setView(view);
        window.clear();
        window.draw(mapSprite);
        for (int i = 0; i < state->numberOfPlayers; i++)
            window.draw(sprites[i]);
        for (int i = 0; i < state->numberOfBullets; i++)
            window.draw(bulletSprites[i]);
        window.display();
    }

    return 0;
}