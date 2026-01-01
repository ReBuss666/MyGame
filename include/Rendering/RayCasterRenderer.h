#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <algorithm>
#include <iostream>
#include "../World/Map.h"
#include "../Utils/settings.h"

class RayCasterRenderer {
public:
    RayCasterRenderer(int screenWidth, int screenHeight)
        : screenWidth_(screenWidth)
        , screenHeight_(screenHeight)
        , renderDistance_(20.f)
        , wallHeight_(1.0f)
        , wallTexture_(nullptr)
    {
        columnVertices_.setPrimitiveType(sf::PrimitiveType::Triangles);
        columnVertices_.resize(screenWidth_ * 6);
    }

    void setTexture(const sf::Texture* texture) {
        wallTexture_ = texture;
    }

    void render(sf::RenderTarget& target, const Map& map,
                sf::Vector2f playerPos, float playerAngle,
                float fov = FOV_RADIANS) {
        
        drawBackground(target);
        float tileSize = map.getTileSize(); // Получаем размер тайла (64)

        for (int x = 0; x < screenWidth_; ++x) {
            float cameraX = 2.0f * x / static_cast<float>(screenWidth_) - 1.0f;
            float rayAngle = playerAngle + std::atan(cameraX * std::tan(fov / 2.0f));
            sf::Vector2f rayDir(std::cos(rayAngle), std::sin(rayAngle));

            float distance = 0.0f;
            bool hitSide = false;

            if (castRay(map, playerPos, rayDir, distance, hitSide)) {
                // 1. Исправление рыбьего глаза
                float correctedDist = distance * std::cos(rayAngle - playerAngle);
                if (correctedDist < 0.01f) correctedDist = 0.01f;

                // 2. ИСПРАВЛЕНИЕ ВЫСОТЫ
                // Дистанция в тайлах. Если стена в 1 тайле от нас, она должна заполнять экран.
                // Убираем умножение на 64 (wallHeight_), оставляем чистую проекцию.
                float fullLineHeight = (static_cast<float>(screenHeight_) / correctedDist);

                // 3. ИСПРАВЛЕНИЕ ТЕКСТУРНЫХ КООРДИНАТ (WallX)
                // Нам нужно найти точную позицию удара в МИРОВЫХ координатах (пикселях)
                // distance - это тайлы, поэтому умножаем на tileSize
                float exactHitX;
                if (hitSide == false) {
                    // Удар в вертикальную грань -> нас интересует Y координата мира
                    exactHitX = playerPos.y + (distance * tileSize) * rayDir.y;
                } else {
                    // Удар в горизонтальную грань -> нас интересует X координата мира
                    exactHitX = playerPos.x + (distance * tileSize) * rayDir.x;
                }
                
                // Переводим мировую координату в локальную координату тайла (0.0 - 1.0)
                float wallX = exactHitX / tileSize;
                wallX -= std::floor(wallX); // Оставляем дробную часть

                // Отражение текстуры (чтобы кирпичи не зеркалились)
                if (hitSide == false && rayDir.x > 0) wallX = 1.0f - wallX;
                if (hitSide == true && rayDir.y < 0) wallX = 1.0f - wallX;

                // 4. Затенение
                float brightness = std::max(0.2f, 1.0f - (correctedDist / renderDistance_));
                if (hitSide) brightness *= 0.7f; 

                sf::Color color(
                    static_cast<uint8_t>(255 * brightness),
                    static_cast<uint8_t>(255 * brightness),
                    static_cast<uint8_t>(255 * brightness)
                );

                drawTexturedColumn(x, fullLineHeight, wallX, color);
            } else {
                drawTexturedColumn(x, 0, 0, sf::Color::Transparent); 
            }
        } 
        
        sf::RenderStates states;
        if (wallTexture_) states.texture = wallTexture_;
        target.draw(columnVertices_, states);
    }
    
    void setRenderDistance(float distance) { renderDistance_ = distance; }
    void setWallHeight(float height) { wallHeight_ = height; }

private:
    int screenWidth_;
    int screenHeight_;
    float renderDistance_;
    float wallHeight_;
    const sf::Texture* wallTexture_;
    sf::VertexArray columnVertices_;

    void drawBackground(sf::RenderTarget& target) {
        sf::RectangleShape sky({static_cast<float>(screenWidth_), static_cast<float>(screenHeight_ / 2)});
        sky.setFillColor(sf::Color(SKY_R, SKY_G, SKY_B));
        target.draw(sky);

        sf::RectangleShape floor({static_cast<float>(screenWidth_), static_cast<float>(screenHeight_ / 2)});
        floor.setPosition({0.f, static_cast<float>(screenHeight_ / 2)});
        floor.setFillColor(sf::Color(FLOOR_R, FLOOR_G, FLOOR_B));
        target.draw(floor);
    }

    void drawTexturedColumn(int x, float fullHeight, float wallX, sf::Color color) {
        float centerY = screenHeight_ / 2.0f;
        float topY = centerY - fullHeight / 2.0f;
        float bottomY = centerY + fullHeight / 2.0f;

        // Обрезаем по экрану
        float drawStart = std::max(0.0f, topY);
        float drawEnd = std::min(static_cast<float>(screenHeight_), bottomY);

        if (drawStart >= drawEnd) {
            int idx = x * 6;
            for(int i=0; i<6; ++i) columnVertices_[idx+i] = sf::Vertex(sf::Vector2f(0,0), sf::Color::Transparent);
            return;
        }

        float textureHeight = wallTexture_ ? static_cast<float>(wallTexture_->getSize().y) : 64.f;
        float textureWidth  = wallTexture_ ? static_cast<float>(wallTexture_->getSize().x) : 64.f;

        // Координата X на текстуре
        float texX = wallX * textureWidth;

        // Координата Y на текстуре (с учетом обрезки, если стена не влезает в экран)
        // d - сколько пикселей стены "ушло" за верхний край экрана
        float d = (drawStart - centerY + fullHeight / 2.0f); 
        
        float texY_start = (d * textureHeight) / fullHeight;
        float heightToDraw = drawEnd - drawStart;
        float texY_end = ((d + heightToDraw) * textureHeight) / fullHeight;

        float left = static_cast<float>(x);
        float right = left + 1.0f;
        int idx = x * 6;

        // Заполняем вершины с правильными UV-координатами
        columnVertices_[idx + 0] = sf::Vertex({left, drawStart}, color, {texX, texY_start});
        columnVertices_[idx + 1] = sf::Vertex({left, drawEnd}, color, {texX, texY_end});
        columnVertices_[idx + 2] = sf::Vertex({right, drawStart}, color, {texX + 1.f, texY_start});
        
        columnVertices_[idx + 3] = columnVertices_[idx + 2];
        columnVertices_[idx + 4] = columnVertices_[idx + 1];
        columnVertices_[idx + 5] = sf::Vertex({right, drawEnd}, color, {texX + 1.f, texY_end});
    }

    bool castRay(const Map& map, sf::Vector2f startPos, sf::Vector2f direction, 
                float& distance, bool& hitSide) {
        float tileSize = map.getTileSize();
        int mapX = static_cast<int>(startPos.x / tileSize);
        int mapY = static_cast<int>(startPos.y / tileSize);

        float deltaDistX = (direction.x == 0) ? 1e30f : std::abs(tileSize / direction.x);
        float deltaDistY = (direction.y == 0) ? 1e30f : std::abs(tileSize / direction.y);
        int stepX = (direction.x < 0) ? -1 : 1;
        int stepY = (direction.y < 0) ? -1 : 1;

        float sideDistX = (direction.x < 0) ? (startPos.x / tileSize - mapX) * deltaDistX : (mapX + 1.0f - startPos.x / tileSize) * deltaDistX;
        float sideDistY = (direction.y < 0) ? (startPos.y / tileSize - mapY) * deltaDistY : (mapY + 1.0f - startPos.y / tileSize) * deltaDistY;

        bool hit = false;
        int maxSteps = static_cast<int>(renderDistance_ * 2); 
        
        for (int step = 0; step < maxSteps && !hit; ++step) {
            if (sideDistX < sideDistY) {
                sideDistX += deltaDistX;
                mapX += stepX;
                hitSide = false;
            } else {
                sideDistY += deltaDistY;
                mapY += stepY;
                hitSide = true;
            }

            if (mapX < 0 || mapX >= map.getWidth() || mapY < 0 || mapY >= map.getHeight()) break;
            if (map.isWall(mapX * tileSize + tileSize / 2, mapY * tileSize + tileSize / 2)) hit = true; 
        }

        if (hit) {
            if(hitSide) distance = (mapY - startPos.y / tileSize + (1 - stepY) / 2) / direction.y;
            else        distance = (mapX - startPos.x / tileSize + (1 - stepX) / 2) / direction.x;
            distance = std::abs(distance);
            return true;
        }
        return false;
    }
};