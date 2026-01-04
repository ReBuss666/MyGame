#pragma once
#include <vector>
#include <SFML/Graphics.hpp>

// x, y
struct Vertex {
    float x, y;
};

struct Sector {
    float floorHeight;
    float ceilHeight;
    sf::Color floorColor;
    sf::Color ceilColor;
    //int texture id потом
};

struct Wall {
    int v1, v2;
    int frontSector;
    int backSector;
    sf::Color color;
};

class Map {
public:
    std::vector<Vertex> vertices;
    std::vector<Sector> sectors;
    std::vector<Wall> walls;
    
    Map() {
        loadTestLevel();
    }
    void loadTestLevel() {
        vertices = {
            {100.f, 100.f}, {400.f, 100.f}, {400.f, 400.f}, {100.f, 400.f}
        };

        sectors.push_back({0.f, 100.f, sf::Color(50, 50, 50), sf::Color(100,100,100)}); //0 пол 100 потолок

        // 4 Стены, все смотрят внутрь сектора 0. backSector = -1 (глухие)
        walls.push_back({0, 1, 0, -1, sf::Color::Red});
        walls.push_back({1, 2, 0, -1, sf::Color::Green});
        walls.push_back({2, 3, 0, -1, sf::Color::Blue});
        walls.push_back({3, 0, 0, -1, sf::Color::Yellow});
    }

    std::pair<sf::Vector2f, sf::Vector2f> getWallCoords(int wallIndex) const {
        return {
            {vertices[walls[wallIndex].v1].x, vertices[walls[wallIndex].v1].y},
            {vertices[walls[wallIndex].v2].x, vertices[walls[wallIndex].v2].y}
        }
    }
};