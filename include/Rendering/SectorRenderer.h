#pragma once
#include <SFML/Graphics.hpp>
#include "../World/SectorMap.h"
#include "../World/Sector.h"
#include "../World/Wall.h"
#include "../Utils/settings.h"
#include "../Core/ResourceManager.h"
#include <unordered_map>
#include <string>

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
    
    // Get texture by name (for per-wall textures from JSON)
    const sf::Texture* getTextureByName(const std::string& textureName) const;

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
    
    // Texture cache for per-wall textures
    mutable std::unordered_map<std::string, const sf::Texture*> textureCache_;
    
    // Batch rendering by texture
    std::unordered_map<const sf::Texture*, std::vector<sf::Vertex>> textureBatches_;

    // Разделение на более мелкие методы
    void drawBackground(sf::RenderTarget& target, const Sector* sector);
    void renderColumn(int x, const RenderContext& context, 
                     const Sector* currentSector, sf::Vector2f playerPos, 
                     sf::Vector2f rayDir, float cosCorrection);
    
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
    
    void renderSolidWall(int x,
                        const ClipRegion& clip,
                        const WallGeometry& geom,
                        sf::Color color,
                        const Wall* wall = nullptr);
    
    void renderFloorAndCeiling(int x, const ClipRegion& clip, 
                               const WallGeometry& geom,
                               sf::Color floorColor, sf::Color ceilColor);
    
    void renderPortalWalls(int x,
                          const ClipRegion& clip,
                          const RenderContext& context,
                          const Sector& currentSector,
                          const Sector& neighborSector,
                          const WallGeometry& geom,
                          sf::Color color,
                          float distance,
                          const Wall* wall = nullptr);
    
    ClipRegion calculatePortalClip(const ClipRegion& currentClip,
                                   float currentCeilingY, float currentFloorY,
                                   float neighborCeilingY, float neighborFloorY) const;
    
    void drawWallSegment(int x,
                        float top, float bottom,
                        const WallGeometry& geom,
                        sf::Color color);
    
    void drawWallSegmentWithTexture(int x,
                                    float top, float bottom,
                                    const WallGeometry& geom,
                                    sf::Color color,
                                    const sf::Texture* texture);
    
    bool checkRayWallIntersection(sf::Vector2f origin,
                                  sf::Vector2f direction,
                                  const Wall& wall,
                                  float& distance,
                                  bool& side) const;
    
    float calculateTextureX(const Wall& wall, sf::Vector2f hitPoint) const;
};
