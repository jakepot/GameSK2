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
    //bool shoot;
    char name[20];
} PlayerInput;

typedef struct GameState {
    int x;
    int y;
} GameState;

typedef struct PlayerState{int x; int y; char name[20]; } PlayerState;
typedef struct AllState{int numberOfPlayers{}; PlayerState players[16]; } AllState;

template<typename T>
sf::Vector2<T> normalize(const sf::Vector2<T> &source) {
    float length = sqrt((source.x * source.x) + (source.y * source.y));
    if (length != 0)
        return sf::Vector2<T>(source.x / length, source.y / length);
    else
        return source;
}

sf::UdpSocket socket;

PlayerInput input = PlayerInput{};

int controls = 0;

/*void sendInput(string name, sf::IpAddress recipient, unsigned short port){
    if (controls == 0) {
        input.left = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
        input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
        input.up = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
        input.down = sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
        //input.shoot = sf::Mouse::isButtonPressed(sf::Mouse::Left);
    }
    else {
        input.left = sf::Keyboard::isKeyPressed(sf::Keyboard::A);
        input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::D);
        input.up = sf::Keyboard::isKeyPressed(sf::Keyboard::W);
        input.down = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
        //input.shoot = sf::Mouse::isButtonPressed(sf::Mouse::Right);
    }

    strcpy(input.name, name.c_str());

    if (input.up || input.right || input.left || input.down) {

        if (socket.send(&input, sizeof input, recipient, port) != sf::Socket::Done) {
            std::cout << "send error" << std::endl;
        }
    }
}*/

int main() {
    string myID;
    getline(cin, myID);
    cout << myID << endl;

    cin >> controls;
//    sf::UdpSocket socket;

    unsigned short startport = 54000;

    while (socket.bind(startport) != sf::Socket::Done) {
        std::cout << "cant bind to " << startport << std::endl;
        //return 1;
        ++startport;
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

    texture.setSmooth(true);
   /* sf::Sprite sprite;
    sprite.setTexture(texture);
    sprite.setPosition(sf::Vector2f(128, 128));
    sprite.setOrigin(sf::Vector2f(128, 128));
    sprite.setScale(0.3f, 0.3f);*/

    vector <sf::Sprite> sprites(16, sf::Sprite(texture));

    //sf::Sprite * sprites[16];
    for (auto &sprite : sprites) {
        //sprite = new sf::Sprite(texture);
        sprite.setPosition(1.0, 2.0);
        sprite.setOrigin(sf::Vector2f(128, 128));
        sprite.setScale(0.3f, 0.3f);
    }

    /*sf::Sprite sprites[16];
    for (int i = 0; i < 16; i++){
        sprites[i].setTexture(texture);
        //s.setPosition(sf::Vector2f(128, 128));
        sprites[i].setOrigin(sf::Vector2f(128, 128));
        sprites[i].setScale(0.3f, 0.3f);
    }*/

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
    if (strcmp(data, "nameinuse") == 0){
        cout << "This name is already in use. Bye bye." << endl;
        return -1;
    }

    cout << "Waiting for the game to start" << endl;

    if (socket.receive(data, 100, received, sender, remotePort) != sf::Socket::Done) {
        std::cout << "receive error" << std::endl;
    }

    strcpy(input.name, myID.c_str());

//    thread t(sendInput, myID, recipient, port);
    // petla gry
    while (window.isOpen()) {
        sf::Event event{};
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();
        }

        //cout << "playing" << endl;


        //
        //sendInput(myID, recipient, port);
        // try again

        if (controls == 0) {
            input.left = sf::Keyboard::isKeyPressed(sf::Keyboard::Left);
            input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::Right);
            input.up = sf::Keyboard::isKeyPressed(sf::Keyboard::Up);
            input.down = sf::Keyboard::isKeyPressed(sf::Keyboard::Down);
            //input.shoot = sf::Mouse::isButtonPressed(sf::Mouse::Left);
        }
        else {
            input.left = sf::Keyboard::isKeyPressed(sf::Keyboard::A);
            input.right = sf::Keyboard::isKeyPressed(sf::Keyboard::D);
            input.up = sf::Keyboard::isKeyPressed(sf::Keyboard::W);
            input.down = sf::Keyboard::isKeyPressed(sf::Keyboard::S);
            //input.shoot = sf::Mouse::isButtonPressed(sf::Mouse::Right);
        }



        if (input.up || input.right || input.left || input.down) {
            cout << "sending input " << endl;
            if (socket.send(&input, sizeof input, recipient, port) != sf::Socket::Done) {
                std::cout << "send error" << std::endl;
            }
        }

        sf::Vector2i mousePos = sf::Mouse::getPosition(window);
        sf::Vector2f aimDirection =
                sf::Vector2f(mousePos) - sf::Vector2f(window.getSize().x / 2, window.getSize().y / 2);

        double radianRotation = atan2(aimDirection.x, -aimDirection.y) / M_PI * 180.0;

        //if (socket.send(&input, sizeof input, recipient, port) != sf::Socket::Done) {


        //break;

        char gameData[2048];
        memset(gameData, 0, sizeof(gameData));
        size_t received1;
        sf::IpAddress sender1;
        unsigned short remotePort1;
        if (socket.receive(gameData, 100, received1, sender1, remotePort1) != sf::Socket::Done) {
            std::cout << "receive error" << std::endl;
        }

        auto *state = (AllState *) gameData;

        /*if (bulletTimer.getElapsedTime().asSeconds() > bulletInterval && input.shoot) {
            aim = normalize(aimDirection);
            bulletSprite.setPosition(sprite.getPosition());
            bulletUp = true;
            bulletTimer.restart();
        } else {
            bulletSprite.move(sf::Vector2f(aim.x * 2, aim.y * 2));
        }*/

        int me = 0;

        for (int i = 0; i < state->numberOfPlayers; i++){
            sprites[i].setPosition(state->players[i].x, state->players[i].y);
            if (strcmp(myID.c_str(), state->players[i].name) == 0)
                me = i;
        }

        //sprite.setPosition(state->x, state->y);
        //sprites[me].setRotation((float) radianRotation);
        view.setCenter(sprites[me].getPosition());
        window.setView(view);
        window.clear();
        window.draw(mapSprite);
        for (int i = 0; i < state->numberOfPlayers; i++)
            window.draw(sprites[i]);
        //if (bulletUp) window.draw(bulletSprite);
        window.display();
    }

    return 0;
}