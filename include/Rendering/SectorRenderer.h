#pragma once
#include <SFML/Graphics.hpp>
#include "../World/SectorMap.h"
#include "../World/Sector.h"
#include "../World/Wall.h"
#include "../Utils/settings.h"

// Структуры для группировки параметров
struct RenderContext {
    float projectionDistance;
    float screenCenterY;
    float playerEyeZ;
    float fov;
};

struct RaycastResult {
    const Wall* wall = nullptr;
    float distance = 0.0f;
    bool hitSide = false;
    const Sector* sector = nullptr;
    bool hit = false;
};

struct ClipRegion {
    float top;
    float bottom;
    
    bool isEmpty() const { return top >= bottom; }
};

struct WallGeometry {
    float topY;
    float bottomY;
    float textureX;
    float screenScaleFactor;
};

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

    // Разделение на более мелкие методы
    void drawBackground(sf::RenderTarget& target, const Sector* sector);
    void renderColumn(int x, const RenderContext& context, 
                     const Sector* currentSector, sf::Vector2f playerPos, 
                     sf::Vector2f rayDir, float cosCorrection,
                     size_t& vertexIndex);
    
    RaycastResult findClosestWall(const Sector& sector, 
                                  sf::Vector2f origin, 
                                  sf::Vector2f direction) const;
    
    WallGeometry calculateWallGeometry(const RenderContext& context,
                                       float distance,
                                       const Sector& sector,
                                       const Wall& wall,
                                       sf::Vector2f hitPoint,
                                       bool hitSide) const;
    
    sf::Color calculateWallColor(float distance, const Sector& sector, bool hitSide) const;
    
    void renderSolidWall(size_t& vertexIndex, int x,
                        const ClipRegion& clip,
                        const WallGeometry& geom,
                        sf::Color color);
    
    void renderPortalWalls(size_t& vertexIndex, int x,
                          const ClipRegion& clip,
                          const RenderContext& context,
                          const Sector& currentSector,
                          const Sector& neighborSector,
                          const WallGeometry& geom,
                          sf::Color color,
                          float distance);
    
    ClipRegion calculatePortalClip(const ClipRegion& currentClip,
                                   float currentCeilingY, float currentFloorY,
                                   float neighborCeilingY, float neighborFloorY) const;
    
    void drawWallSegment(size_t& vertexIndex, int x,
                        float top, float bottom,
                        const WallGeometry& geom,
                        sf::Color color);
    
    bool checkRayWallIntersection(sf::Vector2f origin,
                                  sf::Vector2f direction,
                                  const Wall& wall,
                                  float& distance,
                                  bool& side) const;
    
    float calculateTextureX(const Wall& wall, sf::Vector2f hitPoint) const;
};
