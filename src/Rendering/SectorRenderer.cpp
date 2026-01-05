#include "../../include/Rendering/SectorRenderer.h"
#include <cmath>
#include <algorithm>
#include <iostream>

SectorRenderer::SectorRenderer(int screenWidth, int screenHeight)
    : screenWidth_(screenWidth)
    , screenHeight_(screenHeight)
    , renderDistance_(20.f)
    , wallTexture_(nullptr)
{
    columnVertices_.setPrimitiveType(sf::PrimitiveType::Triangles);
    columnVertices_.resize(screenWidth_ * 6);
    
    std::cout << "[SectorRenderer] Created (" << screenWidth_ << "x" << screenHeight_ << ")" << std::endl;
}

void SectorRenderer::setTexture(const sf::Texture* texture) {
    wallTexture_ = texture;
}

void SectorRenderer::setRenderDistance(float distance) {
    renderDistance_ = distance;
}

void SectorRenderer::render(sf::RenderTarget& target,
                           const SectorMap& map,
                           sf::Vector2f playerPos,
                           float playerAngle,
                           const Sector* currentSector,
                           float fov) {
    
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
            float projectionDistance = (screenHeight_ / 2.0f) / std::tan(fov / 2.0f);
            float wallHeight = (TILE_SIZE * 2.0f * projectionDistance) / correctedDist;

            // Calculate texture coordinate
            sf::Vector2f hitPoint = playerPos + rayDir * distance;
            float wallX = calculateTextureX(*hitWall, hitPoint, hitSide);

            // Calculate brightness based on distance
            float normalizedDist = correctedDist / TILE_SIZE;
            float brightness = std::max(0.3f, 1.0f - (normalizedDist / renderDistance_));
            if (hitSide) brightness *= 0.7f;
            
            // Apply sector light level
            uint8_t sectorLight = hitSector ? hitSector->getLightLevel() : currentSector->getLightLevel();
            brightness *= (sectorLight / 255.0f);

            sf::Color color(
                static_cast<uint8_t>(255 * brightness),
                static_cast<uint8_t>(255 * brightness),
                static_cast<uint8_t>(255 * brightness)
            );

            drawTexturedColumn(x, wallHeight, wallX, color, currentSector);
        } else {
            drawTexturedColumn(x, 0, 0, sf::Color::Transparent, currentSector);
        }
    }

    // Draw all columns in one call
    sf::RenderStates states;
    if (wallTexture_) states.texture = wallTexture_;
    target.draw(columnVertices_, states);
}

void SectorRenderer::drawBackground(sf::RenderTarget& target, const Sector* sector) {
    // Sky (upper half)
    sf::RectangleShape sky;
    sky.setSize({static_cast<float>(screenWidth_), static_cast<float>(screenHeight_ / 2)});
    sky.setFillColor(sf::Color(SKY_R, SKY_G, SKY_B));
    target.draw(sky);

    // Floor (lower half)
    sf::RectangleShape floor;
    floor.setSize({static_cast<float>(screenWidth_), static_cast<float>(screenHeight_ / 2)});
    floor.setPosition({0.f, static_cast<float>(screenHeight_ / 2)});
    
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

void SectorRenderer::drawTexturedColumn(int x, float wallHeight, float wallX, 
                                        sf::Color color, const Sector* sector) {
    
    float centerY = screenHeight_ / 2.0f;
    float topY = centerY - wallHeight / 2.0f;
    float bottomY = centerY + wallHeight / 2.0f;

    // Clip to screen
    float drawStart = std::max(0.0f, topY);
    float drawEnd = std::min(static_cast<float>(screenHeight_), bottomY);

    if (drawStart >= drawEnd) {
        int idx = x * 6;
        for(int i = 0; i < 6; ++i) {
            columnVertices_[idx + i] = sf::Vertex(sf::Vector2f(0, 0), sf::Color::Transparent);
        }
        return;
    }

    // Texture coordinates
    float textureWidth = wallTexture_ ? static_cast<float>(wallTexture_->getSize().x) : 64.f;
    float textureHeight = wallTexture_ ? static_cast<float>(wallTexture_->getSize().y) : 64.f;
    
    // Calculate texture X coordinate - sample from center of texel to avoid bleeding
    float texX = wallX * textureWidth;

    // Calculate texture Y range
    float d = (drawStart - centerY + wallHeight / 2.0f);
    float texY_start = (d * textureHeight) / wallHeight;
    float heightToDraw = drawEnd - drawStart;
    float texY_end = ((d + heightToDraw) * textureHeight) / wallHeight;

    // Fill vertices - each column is 1 pixel wide, samples single texture column
    float left = static_cast<float>(x);
    float right = static_cast<float>(x + 1);
    int idx = x * 6;

    // Use same texture X for entire column (1-pixel wide strip samples 1 texel column)
    columnVertices_[idx + 0] = sf::Vertex({left, drawStart}, color, {texX, texY_start});
    columnVertices_[idx + 1] = sf::Vertex({left, drawEnd}, color, {texX, texY_end});
    columnVertices_[idx + 2] = sf::Vertex({right, drawStart}, color, {texX, texY_start});
    
    columnVertices_[idx + 3] = sf::Vertex({right, drawStart}, color, {texX, texY_start});
    columnVertices_[idx + 4] = sf::Vertex({left, drawEnd}, color, {texX, texY_end});
    columnVertices_[idx + 5] = sf::Vertex({right, drawEnd}, color, {texX, texY_end});
}

bool SectorRenderer::castRay(const Sector& sector,
                             sf::Vector2f origin,
                             sf::Vector2f direction,
                             float& distance,
                             const Wall*& hitWall,
                             bool& hitSide,
                             const Sector** hitSector,
                             int maxDepth) {
    
    float closestDist = renderDistance_ * TILE_SIZE;
    const Wall* closestWall = nullptr;
    bool closestSide = false;
    const Sector* closestSector = &sector;

    for (const auto& wall : sector.getWalls()) {
        float dist = 0.0f;
        bool side = false;
        
        if (rayWallIntersection(origin, direction, wall, dist, side)) {
            if (dist < closestDist && dist > 0.01f) {
                if (wall.isPortal() && maxDepth > 0) {
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

bool SectorRenderer::rayWallIntersection(sf::Vector2f origin,
                                         sf::Vector2f direction,
                                         const Wall& wall,
                                         float& distance,
                                         bool& side) {
    
    sf::Vector2f p1 = wall.getStart();
    sf::Vector2f p2 = wall.getEnd();
    sf::Vector2f wallDir = p2 - p1;

    float denom = direction.x * wallDir.y - direction.y * wallDir.x;
    
    if (std::abs(denom) < 0.0001f) {
        return false;
    }

    sf::Vector2f diff = p1 - origin;
    float t = (diff.x * wallDir.y - diff.y * wallDir.x) / denom;
    float s = (diff.x * direction.y - diff.y * direction.x) / denom;

    if (t > 0.0f && s >= 0.0f && s <= 1.0f) {
        distance = t;
        sf::Vector2f normal = wall.getNormal();
        side = (direction.x * normal.x + direction.y * normal.y) > 0;
        return true;
    }

    return false;
}

float SectorRenderer::calculateTextureX(const Wall& wall, sf::Vector2f hitPoint, bool hitSide) {
    sf::Vector2f start = wall.getStart();
    
    float wallLength = wall.getLength();
    if (wallLength < 0.01f) return 0.0f;
    
    sf::Vector2f toHit = hitPoint - start;
    float distAlongWall = std::sqrt(toHit.x * toHit.x + toHit.y * toHit.y);
    
    // Tile the texture every TILE_SIZE pixels
    float wallX = std::fmod(distAlongWall, TILE_SIZE) / TILE_SIZE;
    
    if (hitSide) {
        wallX = 1.0f - wallX;
    }
    
    return wallX;
}
