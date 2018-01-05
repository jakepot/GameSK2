#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <thread>
#include <iostream>
#include <sstream>
#include <cstring>

using namespace std;

typedef struct PlayerInput {
    bool left;
    bool right;
    bool up;
    bool down;
    bool shoot;
} PlayerInput;

typedef struct GameState {
    int x;
    int y;
} GameState;

typedef struct GameStateM {
    int players;
    int x[16];
    int y[16];
    string names[16];
} GameSt;

template<typename T>
sf::Vector2<T> normalize(const sf::Vector2<T> &source) {
    float length = sqrt((source.x * source.x) + (source.y * source.y));
    if (length != 0)
        return sf::Vector2<T>(source.x / length, source.y / length);
    else
        return source;
}

int main() {
    int myID;
    cin >> myID;
    
    sf::UdpSocket socket;

    if (socket.bind(54000) != sf::Socket::Done) {
        std::cout << "Bind" << std::endl;
        //return 1;
        socket.bind(54001);
    }

    sf::IpAddress recipient = "127.0.0.1";
    unsigned short port = 50000;

    // window setup
    sf::RenderWindow window(sf::VideoMode(800, 600), "Wow ale fajna gra jeszcze w takom nie grauem");
    window.setKeyRepeatEnabled(false);
    window.setVerticalSyncEnabled(true);

    // view setup
    sf::View view(sf::Vector2f(128, 128), sf::Vector2f(400, 300));
    window.setView(view);

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

    sf::Sprite bulletSprite;
    bulletSprite.setTexture(bullet);
    sf::Vector2f aim;
    bool bulletUp = false;
    float bulletInterval = 2.0f;

    std::thread t;

    texture.setSmooth(true);
    sf::Sprite sprite;
    sprite.setTexture(texture);
    sprite.setPosition(sf::Vector2f(128, 128));
    sprite.setOrigin(sf::Vector2f(128, 128));
    sprite.setScale(0.3f, 0.3f);

    sf::Clock bulletTimer;

    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        std::string key;
        PlayerInput input = PlayerInput{};

        input.left = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
        input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
        input.up = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
        input.down = sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
        input.shoot = sf::Mouse::isButtonPressed(sf::Mouse::Left);

        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f aimDirection =
                sf::Vector2f(mousePos) - sf::Vector2f(window.getSize().x / 2, window.getSize().y / 2);

        double radianRotation = atan2(aimDirection.x, -aimDirection.y) / M_PI * 180.0;

        if (socket.send(&input, sizeof input, recipient, port) != sf::Socket::Done) {
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

        auto *state = (GameState *) data;

        if (bulletTimer.getElapsedTime().asSeconds() > bulletInterval && input.shoot) {
            aim = normalize(aimDirection);
            bulletSprite.setPosition(sprite.getPosition());
            bulletUp = true;
            bulletTimer.restart();
        } else {
            bulletSprite.move(sf::Vector2f(aim.x * 2, aim.y * 2));
        }

        sprite.setPosition(state->x, state->y);
        sprite.setRotation((float) radianRotation);
        view.setCenter(state->x, state->y);
        window.setView(view);
        window.clear();
        window.draw(mapSprite);
        window.draw(sprite);
        if (bulletUp) window.draw(bulletSprite);
        window.display();
    }

    return 0;
}