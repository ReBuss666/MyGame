#pragma once
#include <SFML/Graphics.hpp>
#include <cmath>
#include <vector>
#include <algorithm>
#include <iostream>
#include "../World/SectorMap.h"
#include "../World/Sector.h"
#include "../World/Wall.h"
#include "../Utils/settings.h"

/**
 * @brief Basic Sector Renderer (Phase 1: No portals yet)
 * 
 * This is a simplified renderer that only renders the current sector.
 * Portal rendering will be added in Phase 3.
 * 
 * Features:
 * - Renders walls of current sector
 * - Handles floor and ceiling heights
 * - Basic texturing
 * - Distance-based shading
 * 
 * TODO Phase 3:
 * - Portal rendering (see through to neighbor sectors)
 * - Frustum clipping
 * - Upper/lower wall rendering (steps, windows)
 */
class SectorRenderer {
public:
    SectorRenderer(int screenWidth, int screenHeight)
        : screenWidth_(screenWidth)
        , screenHeight_(screenHeight)
        , renderDistance_(20.f)
        , wallTexture_(nullptr)
    {
        columnVertices_.setPrimitiveType(sf::PrimitiveType::Triangles);
        columnVertices_.resize(screenWidth_ * 6);
        
        std::cout << "[SectorRenderer] Created (" << screenWidth_ << "x" << screenHeight_ << ")" << std::endl;
    }

    void setTexture(const sf::Texture* texture) {
        wallTexture_ = texture;
    }

    void setRenderDistance(float distance) {
        renderDistance_ = distance;
    }

    /**
     * @brief Main render method
     * 
     * Renders the view from player's perspective in current sector.
     * 
     * @param target Render target (window or texture)
     * @param map The sector map
     * @param playerPos Player position in world coordinates
     * @param playerAngle Player view angle in radians
     * @param currentSector Pointer to sector player is in (nullptr = render nothing)
     * @param fov Field of view in radians
     */
    void render(sf::RenderTarget& target,
                const SectorMap& map,
                sf::Vector2f playerPos,
                float playerAngle,
                const Sector* currentSector,
                float fov = FOV_RADIANS) {
        
        // Draw background (sky and floor)
        drawBackground(target, currentSector);
        
        if (!currentSector) {
            std::cerr << "[SectorRenderer] WARNING: No current sector to render!" << std::endl;
            return;
        }

        // Render each screen column
        for (int x = 0; x < screenWidth_; ++x) {
            // Calculate ray direction for this column
            float cameraX = 2.0f * x / static_cast<float>(screenWidth_) - 1.0f;
            float rayAngle = playerAngle + std::atan(cameraX * std::tan(fov / 2.0f));
            sf::Vector2f rayDir(std::cos(rayAngle), std::sin(rayAngle));

            // Cast ray and find closest wall hit (follows through portals)
            float distance = 0.0f;
            const Wall* hitWall = nullptr;
            bool hitSide = false;
            const Sector* hitSector = nullptr;

            if (castRay(*currentSector, playerPos, rayDir, distance, hitWall, hitSide, &hitSector)) {
                // Correct fisheye effect
                float correctedDist = distance * std::cos(rayAngle - playerAngle);
                if (correctedDist < 0.1f) correctedDist = 0.1f;

                // Calculate wall height on screen
                // Scale by TILE_SIZE since map coordinates are in pixels (64 per unit)
                float projectionDistance = (screenHeight_ / 2.0f) / std::tan(fov / 2.0f);
                float wallHeight = (TILE_SIZE * 2.0f * projectionDistance) / correctedDist;

                // Calculate texture coordinate
                sf::Vector2f hitPoint = playerPos + rayDir * distance;
                float wallX = calculateTextureX(*hitWall, hitPoint, hitSide);

                // Calculate brightness based on distance (normalize by TILE_SIZE)
                float normalizedDist = correctedDist / TILE_SIZE;
                float brightness = std::max(0.3f, 1.0f - (normalizedDist / renderDistance_));
                if (hitSide) brightness *= 0.7f; // Darken side walls
                
                // Apply sector light level from the sector where wall was hit
                uint8_t sectorLight = hitSector ? hitSector->getLightLevel() : currentSector->getLightLevel();
                brightness *= (sectorLight / 255.0f);

                sf::Color color(
                    static_cast<uint8_t>(255 * brightness),
                    static_cast<uint8_t>(255 * brightness),
                    static_cast<uint8_t>(255 * brightness)
                );

                // Draw wall column
                drawTexturedColumn(x, wallHeight, wallX, color, currentSector);
            } else {
                // No hit - draw empty column
                drawTexturedColumn(x, 0, 0, sf::Color::Transparent, currentSector);
            }
        }

        // Draw all columns in one call
        sf::RenderStates states;
        if (wallTexture_) states.texture = wallTexture_;
        target.draw(columnVertices_, states);
    }

private:
    int screenWidth_;
    int screenHeight_;
    float renderDistance_;
    const sf::Texture* wallTexture_;
    sf::VertexArray columnVertices_;

    /**
     * @brief Draw sky and floor
     */
    void drawBackground(sf::RenderTarget& target, const Sector* sector) {
        // Sky (upper half)
        sf::RectangleShape sky;
        sky.setSize({static_cast<float>(screenWidth_), static_cast<float>(screenHeight_ / 2)});
        sky.setFillColor(sf::Color(SKY_R, SKY_G, SKY_B));
        target.draw(sky);

        // Floor (lower half)
        sf::RectangleShape floor;
        floor.setSize({static_cast<float>(screenWidth_), static_cast<float>(screenHeight_ / 2)});
        floor.setPosition({0.f, static_cast<float>(screenHeight_ / 2)});
        
        // Use sector-specific floor color if available
        if (sector) {
            uint8_t light = sector->getLightLevel();
            floor.setFillColor(sf::Color(FLOOR_R * light / 255, 
                                         FLOOR_G * light / 255, 
                                         FLOOR_B * light / 255));
        } else {
            floor.setFillColor(sf::Color(FLOOR_R, FLOOR_G, FLOOR_B));
        }
        
        target.draw(floor);
    }

    /**
     * @brief Cast a ray and find closest SOLID wall intersection
     * 
     * This version follows through portals to find solid walls.
     * 
     * @param sector Current sector to check
     * @param origin Ray origin (player position)
     * @param direction Ray direction (normalized)
     * @param[out] distance Distance to hit
     * @param[out] hitWall Pointer to wall that was hit
     * @param[out] hitSide Which side was hit (for shading)
     * @param[out] hitSector Sector where the wall was hit
     * @param maxDepth Maximum portal recursion depth
     * @return true if wall was hit, false otherwise
     */
    bool castRay(const Sector& sector,
                 sf::Vector2f origin,
                 sf::Vector2f direction,
                 float& distance,
                 const Wall*& hitWall,
                 bool& hitSide,
                 const Sector** hitSector = nullptr,
                 int maxDepth = 8) {
        
        float closestDist = renderDistance_ * TILE_SIZE;  // Convert to pixel units
        const Wall* closestWall = nullptr;
        bool closestSide = false;
        const Sector* closestSector = &sector;

        // Check intersection with each wall in sector
        for (const auto& wall : sector.getWalls()) {
            float dist = 0.0f;
            bool side = false;
            
            if (rayWallIntersection(origin, direction, wall, dist, side)) {
                if (dist < closestDist && dist > 0.01f) {
                    // Check if this is a portal
                    if (wall.isPortal() && maxDepth > 0) {
                        // Cast ray into neighboring sector
                        Sector* neighbor = wall.getNeighborSector();
                        if (neighbor) {
                            sf::Vector2f newOrigin = origin + direction * (dist + 0.1f);
                            float neighborDist = 0.0f;
                            const Wall* neighborWall = nullptr;
                            bool neighborSide = false;
                            const Sector* neighborHitSector = nullptr;
                            
                            if (castRay(*neighbor, newOrigin, direction, neighborDist, 
                                       neighborWall, neighborSide, &neighborHitSector, maxDepth - 1)) {
                                float totalDist = dist + neighborDist;
                                if (totalDist < closestDist) {
                                    closestDist = totalDist;
                                    closestWall = neighborWall;
                                    closestSide = neighborSide;
                                    closestSector = neighborHitSector;
                                }
                            }
                        }
                    } else {
                        // Solid wall - this is our hit
                        closestDist = dist;
                        closestWall = &wall;
                        closestSide = side;
                        closestSector = &sector;
                    }
                }
            }
        }

        if (closestWall) {
            distance = closestDist;
            hitWall = closestWall;
            hitSide = closestSide;
            if (hitSector) *hitSector = closestSector;
            return true;
        }

        return false;
    }

    /**
     * @brief Check if ray intersects with wall segment
     * 
     * Uses line-line intersection math.
     */
    bool rayWallIntersection(sf::Vector2f origin,
                            sf::Vector2f direction,
                            const Wall& wall,
                            float& distance,
                            bool& side) {
        
        sf::Vector2f p1 = wall.getStart();
        sf::Vector2f p2 = wall.getEnd();
        sf::Vector2f wallDir = p2 - p1;

        // Calculate intersection using parametric line equations
        // Ray: R(t) = origin + t * direction
        // Wall: W(s) = p1 + s * wallDir
        // Solve: origin + t * direction = p1 + s * wallDir

        float denom = direction.x * wallDir.y - direction.y * wallDir.x;
        
        if (std::abs(denom) < 0.0001f) {
            return false; // Lines are parallel
        }

        sf::Vector2f diff = p1 - origin;
        float t = (diff.x * wallDir.y - diff.y * wallDir.x) / denom;
        float s = (diff.x * direction.y - diff.y * direction.x) / denom;

        // Check if intersection is in valid range
        if (t > 0.0f && s >= 0.0f && s <= 1.0f) {
            distance = t;
            
            // Determine which side we hit (for shading)
            sf::Vector2f normal = wall.getNormal();
            side = (direction.x * normal.x + direction.y * normal.y) > 0;
            
            return true;
        }

        return false;
    }

    /**
     * @brief Calculate texture X coordinate for wall hit point
     */
    float calculateTextureX(const Wall& wall, sf::Vector2f hitPoint, bool hitSide) {
        sf::Vector2f start = wall.getStart();
        sf::Vector2f end = wall.getEnd();
        
        // Find position along wall (0.0 to 1.0)
        float wallLength = wall.getLength();
        if (wallLength < 0.01f) return 0.0f;
        
        sf::Vector2f toHit = hitPoint - start;
        float distAlongWall = std::sqrt(toHit.x * toHit.x + toHit.y * toHit.y);
        
        // Tile the texture every TILE_SIZE pixels
        float wallX = std::fmod(distAlongWall, TILE_SIZE) / TILE_SIZE;
        
        // Mirror texture on one side to avoid backwards look
        if (hitSide) {
            wallX = 1.0f - wallX;
        }
        
        return wallX;
    }

    /**
     * @brief Draw a single vertical column of wall
     */
    void drawTexturedColumn(int x, float wallHeight, float wallX, 
                           sf::Color color, const Sector* sector) {
        
        float centerY = screenHeight_ / 2.0f;
        
        // Calculate wall position on screen based on sector floor/ceiling heights
        // For now, assume eye height is at floor + 1.6m
        float eyeHeight = sector ? sector->getFloorHeight() + 1.6f : 1.6f;
        float wallBottomHeight = sector ? sector->getFloorHeight() : 0.0f;
        float wallTopHeight = sector ? sector->getCeilingHeight() : 2.5f;
        
        // Project to screen space
        float topY = centerY - wallHeight / 2.0f;
        float bottomY = centerY + wallHeight / 2.0f;

        // Clip to screen
        float drawStart = std::max(0.0f, topY);
        float drawEnd = std::min(static_cast<float>(screenHeight_), bottomY);

        if (drawStart >= drawEnd) {
            // Wall not visible - draw transparent
            int idx = x * 6;
            for(int i = 0; i < 6; ++i) {
                columnVertices_[idx + i] = sf::Vertex(sf::Vector2f(0, 0), sf::Color::Transparent);
            }
            return;
        }

        // Texture coordinates
        float textureWidth = wallTexture_ ? static_cast<float>(wallTexture_->getSize().x) : 64.f;
        float textureHeight = wallTexture_ ? static_cast<float>(wallTexture_->getSize().y) : 64.f;
        float texX = wallX * textureWidth;

        // Calculate texture Y range
        float d = (drawStart - centerY + wallHeight / 2.0f);
        float texY_start = (d * textureHeight) / wallHeight;
        float heightToDraw = drawEnd - drawStart;
        float texY_end = ((d + heightToDraw) * textureHeight) / wallHeight;

        // Fill vertices
        float left = static_cast<float>(x);
        float right = left + 1.0f;
        int idx = x * 6;

        columnVertices_[idx + 0] = sf::Vertex({left, drawStart}, color, {texX, texY_start});
        columnVertices_[idx + 1] = sf::Vertex({left, drawEnd}, color, {texX, texY_end});
        columnVertices_[idx + 2] = sf::Vertex({right, drawStart}, color, {texX + 1.f, texY_start});
        
        columnVertices_[idx + 3] = columnVertices_[idx + 2];
        columnVertices_[idx + 4] = columnVertices_[idx + 1];
        columnVertices_[idx + 5] = sf::Vertex({right, drawEnd}, color, {texX + 1.f, texY_end});
    }
};