#include "../../include/Rendering/SectorRenderer.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <vector>

SectorRenderer::SectorRenderer(int screenWidth, int screenHeight)
    : screenWidth_(screenWidth)
    , screenHeight_(screenHeight)
    , renderDistance_(20.f)
    , currentPitch_(0.0f)
    , wallTexture_(nullptr)
{
    columnVertices_.setPrimitiveType(sf::PrimitiveType::Triangles);
    columnVertices_.resize(screenWidth_ * 6 * 10);
    
    floorCeilingVertices_.setPrimitiveType(sf::PrimitiveType::Triangles);
    floorCeilingVertices_.resize(screenWidth_ * 12);
    
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
                           float fov,
                           float playerHeight,
                           float pitch) {
    
    currentPitch_ = pitch;
    
    drawBackground(target, currentSector);
    
    if (!currentSector) {
        std::cerr << "[SectorRenderer] WARNING: No current sector to render!" << std::endl;
        return;
    }

    float playerEyeZ = currentSector->getFloorHeight() + playerHeight;
    
    float projDist = (screenHeight_ / 2.0f) / std::tan(fov / 2.0f);
    
    float pitchOffset = pitch * screenHeight_ * 0.5f;
    float screenCenterY = screenHeight_ / 2.0f - pitchOffset;
    
    const float WORLD_SCALE = TILE_SIZE;

    size_t vertexIndex = 0;

    for (int x = 0; x < screenWidth_; ++x) {
        float cameraX = 2.0f * x / static_cast<float>(screenWidth_) - 1.0f;
        float rayAngle = playerAngle + std::atan(cameraX * std::tan(fov / 2.0f));
        float cosCorrection = std::cos(rayAngle - playerAngle);
        sf::Vector2f rayDir(std::cos(rayAngle), std::sin(rayAngle));

        float clipTop = 0.0f;
        float clipBottom = static_cast<float>(screenHeight_);
        
        const Sector* sector = currentSector;
        sf::Vector2f rayOrigin = playerPos;
        float totalDistance = 0.0f;
        int maxPortals = 8;  // Максимальная глубина порталов

        for (int portalDepth = 0; portalDepth < maxPortals && sector != nullptr; ++portalDepth) {
            float closestDist = renderDistance_ * TILE_SIZE;
            const Wall* closestWall = nullptr;
            bool closestSide = false;

            for (const auto& wall : sector->getWalls()) {
                float dist = 0.0f;
                bool side = false;
                
                if (rayWallIntersection(rayOrigin, rayDir, wall, dist, side)) {
                    if (dist > 0.01f && dist < closestDist) {
                        closestDist = dist;
                        closestWall = &wall;
                        closestSide = side;
                    }
                }
            }

            if (!closestWall) break;

            totalDistance += closestDist;
            float correctedDist = totalDistance * cosCorrection;
            if (correctedDist < 0.1f) correctedDist = 0.1f;

            float currentFloor = sector->getFloorHeight();
            float currentCeiling = sector->getCeilingHeight();

            float currentFloorY = screenCenterY - (currentFloor - playerEyeZ) * projDist * WORLD_SCALE / correctedDist;
            float currentCeilingY = screenCenterY - (currentCeiling - playerEyeZ) * projDist * WORLD_SCALE / correctedDist;

            float wallScreenHeight = currentFloorY - currentCeilingY;
            if (wallScreenHeight < 0.1f) wallScreenHeight = 0.1f;

            float textureHeight = wallTexture_ ? static_cast<float>(wallTexture_->getSize().y) : 64.f;
            float zoomfactor = 2.f;
            float screenScaleFactor = (textureHeight * zoomfactor) / wallScreenHeight;

            sf::Vector2f hitPoint = rayOrigin + rayDir * closestDist;
            float wallX = calculateTextureX(*closestWall, hitPoint, closestSide);

            float worldDist = correctedDist / TILE_SIZE;
            float brightness = std::max(0.3f, 1.0f - (worldDist / renderDistance_));
            if (closestSide) brightness *= 0.7f;
            brightness *= (sector->getLightLevel() / 255.0f);

            sf::Color wallColor(
                static_cast<uint8_t>(255 * brightness),
                static_cast<uint8_t>(255 * brightness),
                static_cast<uint8_t>(255 * brightness)
            );

            if (closestWall->isPortal()) {
                Sector* neighbor = closestWall->getNeighborSector();
                if (!neighbor) {
                    drawWallSegment(vertexIndex, x, 
                                   std::max(clipTop, currentCeilingY),
                                   std::min(clipBottom, currentFloorY),
                                   currentCeilingY, screenScaleFactor,
                                   wallX, wallColor);
                    break;
                }

                float neighborFloor = neighbor->getFloorHeight();
                float neighborCeiling = neighbor->getCeilingHeight();

                float neighborFloorY = screenCenterY - (neighborFloor - playerEyeZ) * projDist * WORLD_SCALE / correctedDist;
                float neighborCeilingY = screenCenterY - (neighborCeiling - playerEyeZ) * projDist * WORLD_SCALE / correctedDist;

                if (neighborCeiling < currentCeiling) {
                    float upperTop = std::max(clipTop, currentCeilingY);
                    float upperBottom = std::min(clipBottom, neighborCeilingY);
                    
                    if (upperTop < upperBottom) {
                        drawWallSegment(vertexIndex, x, upperTop, upperBottom, 
                                       currentCeilingY, screenScaleFactor, wallX, wallColor);
                    }
                }

                if (neighborFloor > currentFloor) {
                    float lowerTop = std::max(clipTop, neighborFloorY);
                    float lowerBottom = std::min(clipBottom, currentFloorY);
                    
                    if (lowerTop < lowerBottom) {
                        drawWallSegment(vertexIndex, x, lowerTop, lowerBottom, 
                                       currentCeilingY, screenScaleFactor, wallX, wallColor);
                    }
                }

                float portalTop = std::max(currentCeilingY, neighborCeilingY);
                float portalBottom = std::min(currentFloorY, neighborFloorY);
                
                clipTop = std::max(clipTop, portalTop);
                clipBottom = std::min(clipBottom, portalBottom);

                if (clipTop >= clipBottom) break;

                rayOrigin = hitPoint + rayDir * 0.1f;
                sector = neighbor;

            } else {
                float wallTop = std::max(clipTop, currentCeilingY);
                float wallBottom = std::min(clipBottom, currentFloorY);
                
                if (wallTop < wallBottom) {
                    drawWallSegment(vertexIndex, x, wallTop, wallBottom, 
                                   currentCeilingY, screenScaleFactor, wallX, wallColor);
                }
                break;
            }
        }
    }

    columnVertices_.resize(vertexIndex);

    sf::RenderStates states;
    if (wallTexture_) states.texture = wallTexture_;
    target.draw(columnVertices_, states);

    columnVertices_.resize(screenWidth_ * 6 * 10);
}

void SectorRenderer::drawWallSegment(size_t& vertexIndex, int x,
                                     float top, float bottom, 
                                     float anchorTopY, float screenScaleFactor,
                                     float wallX, sf::Color color) {
    if (top >= bottom) return;
    if (vertexIndex + 6 > columnVertices_.getVertexCount()) return;
    
    float textureWidth = wallTexture_ ? static_cast<float>(wallTexture_->getSize().x) : 64.f;
    float textureHeight = wallTexture_ ? static_cast<float>(wallTexture_->getSize().y) : 64.f;
    
    float texX = wallX * textureWidth;
    
    float texYStart = (top - anchorTopY) * screenScaleFactor;
    float texYEnd = (bottom - anchorTopY) * screenScaleFactor;

    float left = static_cast<float>(x);
    float right = left + 1.f;

    if (vertexIndex + 6 > columnVertices_.getVertexCount()) {
        return;
    }

    columnVertices_[vertexIndex + 0] = sf::Vertex({left, top}, color, {texX, texYStart});
    columnVertices_[vertexIndex + 1] = sf::Vertex({left, bottom}, color, {texX, texYEnd});
    columnVertices_[vertexIndex + 2] = sf::Vertex({right, top}, color, {texX, texYStart});
    
    columnVertices_[vertexIndex + 3] = sf::Vertex({right, top}, color, {texX, texYStart});
    columnVertices_[vertexIndex + 4] = sf::Vertex({left, bottom}, color, {texX, texYEnd});
    columnVertices_[vertexIndex + 5] = sf::Vertex({right, bottom}, color, {texX, texYEnd});
    
    vertexIndex += 6;
}

void SectorRenderer::drawBackground(sf::RenderTarget& target, const Sector* sector) {
    float pitchOffset = currentPitch_ * screenHeight_ * 0.5f;
    float horizonY = screenHeight_ / 2.0f - pitchOffset;
    
    sf::RectangleShape sky;
    sky.setSize({static_cast<float>(screenWidth_), horizonY});
    sky.setFillColor(sf::Color(SKY_R, SKY_G, SKY_B));
    target.draw(sky);

    sf::VertexArray floorGradient(sf::PrimitiveType::Triangles, 6);
    
    float floorHeight = static_cast<float>(screenHeight_) - horizonY;
    
    sf::Color farColor(FLOOR_R / 4, FLOOR_G / 4, FLOOR_B / 4);
    sf::Color nearColor(FLOOR_R, FLOOR_G, FLOOR_B);
    
    floorGradient[0] = sf::Vertex({0.f, horizonY}, farColor);
    floorGradient[1] = sf::Vertex({0.f, static_cast<float>(screenHeight_)}, nearColor);
    floorGradient[2] = sf::Vertex({static_cast<float>(screenWidth_), horizonY}, farColor);
    
    floorGradient[3] = sf::Vertex({static_cast<float>(screenWidth_), horizonY}, farColor);
    floorGradient[4] = sf::Vertex({0.f, static_cast<float>(screenHeight_)}, nearColor);
    floorGradient[5] = sf::Vertex({static_cast<float>(screenWidth_), static_cast<float>(screenHeight_)}, nearColor);
    
    target.draw(floorGradient);
}

void SectorRenderer::drawTexturedColumn(int x, float wallTopY, float wallBottomY, float wallX, 
                                        sf::Color color, float texYStart, float texYEnd) {
    float drawStart = std::max(0.0f, wallTopY);
    float drawEnd = std::min(static_cast<float>(screenHeight_), wallBottomY);

    if (drawStart >= drawEnd || wallTopY >= wallBottomY) {
        return;
    }

    float textureWidth = wallTexture_ ? static_cast<float>(wallTexture_->getSize().x) : 64.f;
    float textureHeight = wallTexture_ ? static_cast<float>(wallTexture_->getSize().y) : 64.f;
    
    float texX = wallX * textureWidth;
    float wallScreenHeight = wallBottomY - wallTopY;
    float wallWorldHeight = texYEnd - texYStart;
    
    float topClipRatio = (drawStart - wallTopY) / wallScreenHeight;
    float bottomClipRatio = (drawEnd - wallTopY) / wallScreenHeight;
    
    float texY_start = (texYStart + topClipRatio * wallWorldHeight) * textureHeight / 3.0f;
    float texY_end = (texYStart + bottomClipRatio * wallWorldHeight) * textureHeight / 3.0f;

    float left = static_cast<float>(x);
    float right = static_cast<float>(x + 1);
    int idx = x * 6;

    columnVertices_[idx + 0] = sf::Vertex({left, drawStart}, color, {texX, texY_start});
    columnVertices_[idx + 1] = sf::Vertex({left, drawEnd}, color, {texX, texY_end});
    columnVertices_[idx + 2] = sf::Vertex({right, drawStart}, color, {texX, texY_start});
    
    columnVertices_[idx + 3] = sf::Vertex({right, drawStart}, color, {texX, texY_start});
    columnVertices_[idx + 4] = sf::Vertex({left, drawEnd}, color, {texX, texY_end});
    columnVertices_[idx + 5] = sf::Vertex({right, drawEnd}, color, {texX, texY_end});
}

void SectorRenderer::drawFloorCeilingColumn(int x, float topY, float bottomY, sf::Color color, bool isFloor) {
    float drawStart = std::max(0.0f, topY);
    float drawEnd = std::min(static_cast<float>(screenHeight_), bottomY);
    
    float left = static_cast<float>(x);
    float right = static_cast<float>(x + 1);
    
    int idx = x * 12 + (isFloor ? 0 : 6);
    
    if (drawStart >= drawEnd) {
        for(int i = 0; i < 6; ++i) {
            floorCeilingVertices_[idx + i] = sf::Vertex(sf::Vector2f(0, 0), sf::Color::Transparent);
        }
        return;
    }
    
    floorCeilingVertices_[idx + 0] = sf::Vertex({left, drawStart}, color);
    floorCeilingVertices_[idx + 1] = sf::Vertex({left, drawEnd}, color);
    floorCeilingVertices_[idx + 2] = sf::Vertex({right, drawStart}, color);
    
    floorCeilingVertices_[idx + 3] = sf::Vertex({right, drawStart}, color);
    floorCeilingVertices_[idx + 4] = sf::Vertex({left, drawEnd}, color);
    floorCeilingVertices_[idx + 5] = sf::Vertex({right, drawEnd}, color);
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

float SectorRenderer::calculateTextureX(const Wall& wall, sf::Vector2f hitPoint, bool /*hitSide*/) {
    sf::Vector2f start = wall.getStart();
    sf::Vector2f end = wall.getEnd();
    
    sf::Vector2f wallDir = end - start;
    float wallLengthSq = wallDir.x * wallDir.x + wallDir.y * wallDir.y;
    if (wallLengthSq < 0.0001f) return 0.0f;
    
    sf::Vector2f toHit = hitPoint - start;
    float t = (toHit.x * wallDir.x + toHit.y * wallDir.y) / wallLengthSq;
    
    float wallLength = std::sqrt(wallLengthSq);
    float distAlongWall = t * wallLength;
    
    float wallX = std::fmod(distAlongWall, TILE_SIZE) / TILE_SIZE;
    if (wallX < 0.0f) wallX += 1.0f;
    
    return wallX;
}
