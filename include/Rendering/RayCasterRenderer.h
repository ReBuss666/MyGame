#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include "World/map.h"
#include "Utils/settings.h"

class RayCasterRenderer {
public:
    RayCasterRenderer(int screenWidth, int screenHeight)
        : screenWidth_(screenWidth)
        , screenHeight_(screenHeight)
        , renderDistance_(20.f)
        , wallHeight_(1.0f)
    {
        //vertex massive for columns (2 triangles per column = 6 vertices)    
        columnVertices_.setPrimitiveType(sf::PrimitiveType::Triangles);
        columnVertices_.resize(screenWidth_ * 6);

        std::cout << "[RayCasterRenderer] Initialized with screen size: "
                  << screenWidth_ << "x" << screenHeight_ << std::endl;
    }

    // Render the 3D view using raycasting
    void render(sf::RenderWindow& window, const Map& map,
                sf::Vector2f playerPos, float playerAngle,
                float fov = FOV_RADIANS) {
        
        // Clear previous frame's vertices
        drawBackground(window);

        //cast rays for each column
        for (int x = 0; x < screenWidth_; ++x) {
            // calculating ray angle
            float cameraX = 2.0f * x / static_cast<float>(screenWidth_) - 1.0f; // from -1 to 1
            float rayAngle = playerAngle + std::atan(cameraX * std::tan(fov / 2.0f));
            
            // ray direction
            sf::Vector2f rayDir(std::cos(rayAngle), std::sin(rayAngle));

            // raycast for wall hit
            float distance = 0.0f;
            sf::Color wallColor; // default wall color
            if (castRay(map, playerPos, rayDir, distance, wallColor)) {
                // Исправляем fish-eye эффект
                float correctedDist = distance * std::cos(rayAngle - playerAngle);
                
                float distanceInPixels = correctedDist * map.getTileSize();

                // Вычисляем высоту колонны на экране
                float columnHeight = (screenHeight_ * wallHeight_) / distanceInPixels;
                
                // Ограничиваем высоту
                if (columnHeight > screenHeight_ * 2.0f) {
                    columnHeight = screenHeight_ * 2.0f;
                }
                
                // Затемнение по расстоянию
                float brightness = std::max(0.2f, 1.0f - (correctedDist / renderDistance_));
                sf::Color shadedColor(
                    static_cast<uint8_t>(wallColor.r * brightness),
                    static_cast<uint8_t>(wallColor.g * brightness),
                    static_cast<uint8_t>(wallColor.b * brightness)
                );
                
                // Рисуем колонну
                drawColumn(x, columnHeight, shadedColor);
            }
        } 
        // rendering all columns at once
        window.draw(columnVertices_);
    }
    
    void setRenderDistance(float distance) { renderDistance_ = distance; }
    void setWallHeight(float height) { wallHeight_ = height; }

private:
    int screenWidth_;
    int screenHeight_;
    float renderDistance_;
    float wallHeight_;

    sf::VertexArray columnVertices_;

    void drawBackground(sf::RenderWindow& window) {
        sf::RectangleShape sky({static_cast<float>(screenWidth_), static_cast<float>(screenHeight_ / 2)});
        sky.setPosition({0.f, 0.f});
        sky.setFillColor(sf::Color(SKY_R, SKY_G, SKY_B));
        window.draw(sky);

        // Draw floor
        sf::RectangleShape floor({static_cast<float>(screenWidth_), static_cast<float>(screenHeight_ / 2)});
        floor.setPosition({0.f, static_cast<float>(screenHeight_ / 2)});
        floor.setFillColor(sf::Color(FLOOR_R, FLOOR_G, FLOOR_B));
        window.draw(floor);
    }

    //vertical column rendering to vertex array
    void drawColumn(int x, float height, sf::Color color) {
        float halfHeight = height / 2.0f;
        float CenterY = screenHeight_ / 2.0f;

        float top = CenterY - halfHeight;
        float bottom = CenterY + halfHeight;

        //block out of screen
        if (top < 0) top = 0;
        if (bottom >screenHeight_) bottom = screenHeight_;

        float left = static_cast<float>(x);
        float right = left + 1.0f;

        int idx = x * 6;

        //first triangle 
        columnVertices_[idx + 0].position = sf::Vector2f{left, top};
        columnVertices_[idx + 0].color = color;

        columnVertices_[idx + 1].position = sf::Vector2f{left, bottom};
        columnVertices_[idx + 1].color = color;
        
        columnVertices_[idx + 2].position = sf::Vector2f{right, top};
        columnVertices_[idx + 2].color = color;
        
        //second triangle

        columnVertices_[idx + 3].position = sf::Vector2f{right, top};
        columnVertices_[idx + 3].color = color;
        
        columnVertices_[idx + 4].position = sf::Vector2f{left, bottom};
        columnVertices_[idx + 4].color = color;

        columnVertices_[idx + 5].position = sf::Vector2f{right, bottom};
        columnVertices_[idx + 5].color = color;
    }

    bool castRay(const Map& map, sf::Vector2f startPos, sf::Vector2f direction, 
                float& distance, sf::Color& wallColor) {
        float tileSize = map.getTileSize();

        // grid coordinates
        int mapX = static_cast<int>(startPos.x / tileSize);
        int mapY = static_cast<int>(startPos.y / tileSize);

        // Length of ray from one x or y side to next x or y side
        float deltaDistX = (direction.x == 0) ? 1e30f : std::abs(tileSize / direction.x);
        float deltaDistY = (direction.y == 0) ? 1e30f : std::abs(tileSize / direction.y);

        // Step direction and initial sideDist
        int stepX = (direction.x < 0) ? -1 : 1;
        int stepY = (direction.y < 0) ? -1 : 1;

        // Calculate initial side distances
        float sideDistX, sideDistY;

        // Starting side distances 
        float posInTileX = (startPos.x / tileSize) - mapX;
        float posInTileY = (startPos.y / tileSize) - mapY;

        if (direction.x < 0) {
            sideDistX = posInTileX * deltaDistX;
        } else {
            sideDistX = (1.0f - posInTileX) * deltaDistX;
        }

        if (direction.y < 0) {
            sideDistY = posInTileY * deltaDistY;
        } else {
            sideDistY = (1.0f - posInTileY) * deltaDistY;
        }
        // Perform DDA
        bool hit = false;
        bool side = false;
        int maxSteps = static_cast<int>(renderDistance_ * 2); // Limit steps to avoid infinite loops
        
        for (int step = 0; step < maxSteps && !hit; ++step) {
            // Calculate the next step
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                side = false;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                side = true;
            }

            // Check if ray has hit a wall
            if (mapX < 0 || mapX >= map.getWidth() ||
            mapY < 0 || mapY >= map.getHeight()) {
                break;
            }

            // Check if the ray has hit a wall
            if (map.isWall(mapX * tileSize + tileSize / 2, mapY * tileSize + tileSize / 2)) {
                hit = true; 
            }
        }

        if (hit) {
            if (side) {
                distance = (mapY - startPos.y / tileSize + (1 - stepY) / 2) / direction.y;
            } else {
                distance = (mapX - startPos.x / tileSize + (1 - stepX) / 2) / direction.x;
            }

            // absolute distance
            distance = std::abs(distance);

            // different colors
            if (side) {
                wallColor = sf::Color(180, 50, 50); // Darker for y-sides
            } else {
                wallColor = sf::Color(255, 80, 80); // Brighter for x-sides
            }

            return true;
        }

        return false;
    }
};