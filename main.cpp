#include <SFML/Graphics.hpp>
#include <SFML/Network.hpp>
#include <thread>
#include <iostream>
#include <sstream>
#include <cstring>

using namespace std;
typedef struct PlayerInput{bool left; bool right; bool up; bool down; } PlayerInput;
typedef struct GameState{int x; int y; } GameState;

int main()
{
    sf::UdpSocket socket;

    if (socket.bind(54000) != sf::Socket::Done)
    {
        std::cout << "Bind" << std::endl;
        return 1;
    }

    sf::IpAddress recipient = "127.0.0.1";
    unsigned short port = 50000;

    // window setup
    sf::RenderWindow window(sf::VideoMode(800, 600), "Wow ale fajna gra jeszcze w takom nie grauem");
    window.setKeyRepeatEnabled(false);
    window.setVerticalSyncEnabled(true);

    // view setup
    sf::View view(sf::Vector2f(128, 128), sf::Vector2f(400,300));
    window.setView(view);

    sf::Texture texture;

    if (!texture.loadFromFile("resources/Pacman 3.png"))
    {
      return 0;
    }
    sf::Texture map;
    if (!map.loadFromFile("resources/map.png"))
    {
        return 0;
    }

    sf::Sprite mapSprite;
    mapSprite.setTexture(map);

    std::thread t;

    texture.setSmooth(true);
    sf::Sprite sprite;
    sprite.setTexture(texture);
    sprite.setPosition(sf::Vector2f(128, 128));
    sprite.setOrigin(sf::Vector2f(128,128));
    sprite.setScale(0.3f,0.3f);

    while (window.isOpen())
    {
        sf::Event event{};
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::Closed)
                window.close();
        }
        std::string key;
        PlayerInput input = PlayerInput{};

        input.left = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
        input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
        input.up = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
        input.down = sf::Keyboard::isKeyPressed(sf::Keyboard::Down);

        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2i aimDirection = mousePos - sf::Vector2i(window.getSize().x/2, window.getSize().y/2);

        double radianRotation = atan2(aimDirection.x, -aimDirection.y) / M_PI * 180.0;

        if (socket.send(&input, sizeof input, recipient, port) != sf::Socket::Done)
        {
            std::cout << "send error" << std::endl;
        }

        char data[100];
        memset(data, 0, sizeof(data));
        size_t received;
        sf::IpAddress sender;
        unsigned short remotePort;
        if (socket.receive(data, 100, received, sender, remotePort) != sf::Socket::Done)
        {
            std::cout << "receive error" << std::endl;
        }

        auto * state = (GameState*) data;

        sprite.setPosition(state->x, state->y);
        sprite.setRotation((float)radianRotation);
        view.setCenter(state->x, state->y);
        window.setView(view);
        window.clear();
        window.draw(mapSprite);
        window.draw(sprite);
        window.display();
    }

    return 0;
}