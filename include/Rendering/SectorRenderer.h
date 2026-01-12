#pragma once
#include <SFML/Graphics.hpp>
#include "../World/SectorMap.h"
#include "../World/Sector.h"
#include "../World/Wall.h"
#include "../Utils/settings.h"

class SectorRenderer {
public:
    SectorRenderer(int screenWidth, int screenHeight);
    ~SectorRenderer() = default;

    void setTexture(const sf::Texture* texture);
    void setRenderDistance(float distance);

    void render(sf::RenderTarget& target,
                const SectorMap& map,
                sf::Vector2f playerPos,
                float playerAngle,
                const Sector* currentSector,
                float fov = FOV_RADIANS,
                float playerHeight = 0.5f,
                float pitch = 0.0f);

private:
    int screenWidth_;
    int screenHeight_;
    float renderDistance_;
    float currentPitch_;
    const sf::Texture* wallTexture_;
    sf::VertexArray columnVertices_;
    sf::VertexArray floorCeilingVertices_;

    void drawBackground(sf::RenderTarget& target, const Sector* sector);
    void drawTexturedColumn(int x, float wallTopY, float wallBottomY, float wallX, 
                           sf::Color color, float texYStart, float texYEnd);
    void drawFloorCeilingColumn(int x, float topY, float bottomY, sf::Color color, bool isFloor);
    
    void drawWallSegment(size_t& vertexIndex, int x, float top, float bottom, 
                        float anchorTopY, float screenScaleFactor,
                        float wallX, sf::Color color);

    bool castRay(const Sector& sector,
                 sf::Vector2f origin,
                 sf::Vector2f direction,
                 float& distance,
                 const Wall*& hitWall,
                 bool& hitSide,
                 const Sector** hitSector = nullptr,
                 int maxDepth = 8);

    bool rayWallIntersection(sf::Vector2f origin,
                            sf::Vector2f direction,
                            const Wall& wall,
                            float& distance,
                            bool& side);

    float calculateTextureX(const Wall& wall, sf::Vector2f hitPoint, bool hitSide);
};
