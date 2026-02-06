#pragma once
#include "Sector.h"
#include "Wall.h"
#include "../Utils/settings.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <iostream>

class SectorMap {
public:
    struct HitResult {
        bool hit;
        Wall* wall;
        sf::Vector2f point;
        float distance;
        Sector* sector;
        float distAlongWall;
    };
    
    // Perform a raycast through portal sectors
    HitResult raycast(sf::Vector2f origin, sf::Vector2f direction, float maxDist = 100.0f) {
        HitResult result = {false, nullptr, {}, 0.0f, nullptr, 0.0f};
        
        Sector* currentSector = findSectorAt(origin);
        if (!currentSector) return result;
        
        float currentDist = 0.0f;
        sf::Vector2f currentPos = origin;
        
        int iterations = 0;
        const int MAX_ITERATIONS = 50; // Prevent infinite loops
        
        while (currentDist < maxDist && iterations < MAX_ITERATIONS) {
            iterations++;
            
            // Checks Walls in current sector
            Wall* closestWall = nullptr;
            float closestDist = maxDist;
            sf::Vector2f closestHit;
            
            for (auto& wall : currentSector->getWalls()) {
                sf::Vector2f p1 = wall.getStart();
                sf::Vector2f p2 = wall.getEnd();
                
                // Intersection check (Ray vs Line Segment)
                sf::Vector2f v1 = p1 - currentPos;
                sf::Vector2f v2 = p2 - currentPos;
                sf::Vector2f v3 = {-direction.y, direction.x}; // Normal to ray
                
                float dot1 = v1.x * v3.x + v1.y * v3.y;
                float dot2 = v2.x * v3.x + v2.y * v3.y;
                
                // If distinct signs, points are on opposite sides of ray line
                if ((dot1 > 0 && dot2 < 0) || (dot1 < 0 && dot2 > 0)) {
                    float t1 = (v1.x * direction.y - v1.y * direction.x) / (dot1 - dot2);
                    // Actual intersection time 't' along ray
                     // Line eq: P = Origin + dir * t
                     // Wall eq: P = P1 + (P2-P1)*u
                     // Determinant method
                     float det = direction.x * (p2.y - p1.y) - direction.y * (p2.x - p1.x);
                     if (std::abs(det) < 0.0001f) continue;
                     
                     float t = ((p1.x - currentPos.x) * (p2.y - p1.y) - (p1.y - currentPos.y) * (p2.x - p1.x)) / det;
                     float u = ((p1.x - currentPos.x) * direction.y - (p1.y - currentPos.y) * direction.x) / det;
                     
                     if (t > 0.001f && t < closestDist && u >= 0.0f && u <= 1.0f) {
                         closestDist = t;
                         closestWall = &wall;
                         closestHit = currentPos + direction * t;
                     }
                }
            }
            
            if (closestWall) {
                // Determine actual distance including previous sectors
                float totalDist = currentDist + closestDist;
                if (totalDist > maxDist) break;
                
                if (closestWall->isPortal()) {
                     Sector* next = closestWall->getNeighborSector();
                     if (next) {
                         currentDist += closestDist;
                         currentPos = closestHit + direction * 0.01f; // Nudge forward
                         currentSector = next;
                         continue;
                     }
                }
                
                // Hit a solid wall
                result.hit = true;
                result.wall = closestWall;
                result.distance = totalDist;
                result.point = closestHit;
                result.sector = currentSector;
                
                // Calculate distance along wall
                sf::Vector2f toHit = closestHit - closestWall->getStart();
                sf::Vector2f wallDir = closestWall->getEnd() - closestWall->getStart();
                float length = closestWall->getLength();
                if (length > 0) {
                     result.distAlongWall = (toHit.x * wallDir.x + toHit.y * wallDir.y) / length;
                     // Is already unit normalized? No, dot product is projection * length
                     // result.distAlongWall = projection / length * length?
                     // No, dot(A, B) = |A||B|cos(theta).
                     // Projection len = dot(toHit, wallDir/len).
                     // distAlong = dot(toHit, wallDir/len).
                     // But formula above:
                     // dot(toHit, wallDir) = |toHit| |wallDir| cos(theta)
                     // / length (which is |wallDir|) -> |toHit| cos(theta) * |wallDir|.
                     result.distAlongWall = (toHit.x * wallDir.x + toHit.y * wallDir.y) / (length * length) * length; 
                     // Wait, simplest:
                     // dist = sqrt(toHit.sqmag)
                     result.distAlongWall = std::sqrt(toHit.x*toHit.x + toHit.y*toHit.y);
                } else {
                    result.distAlongWall = 0;
                }

                return result;
            } else {
                break; // No wall hit in current sector (shouldn't happen in closed sectors)
            }
        }
        
        return result;
    }

    SectorMap() = default;

    void addSector(const Sector& sector) {
        int id = sector.getId();
        sectors_[id] = sector;
        std::cout << "[SectorMap] Added sector " << id 
                  << " (floor: " << sector.getFloorHeight() 
                  << ", ceiling: " << sector.getCeilingHeight() << ")" << std::endl;
    }

    Sector* getSector(int id) {
        auto it = sectors_.find(id);
        return (it != sectors_.end()) ? &it->second : nullptr;
    }

    const Sector* getSector(int id) const {
        auto it = sectors_.find(id);
        return (it != sectors_.end()) ? &it->second : nullptr;
    }

    size_t getSectorCount() const {
        return sectors_.size();
    }

    std::unordered_map<int, Sector>& getSectors() {
        return sectors_;
    }

    const std::unordered_map<int, Sector>& getSectors() const {
        return sectors_;
    }

    Sector* findSectorAt(sf::Vector2f point) {
        for (auto& [id, sector] : sectors_) {
            if (sector.containsPoint(point)) {
                return &sector;
            }
        }
        return nullptr;
    }

    const Sector* findSectorAt(sf::Vector2f point) const {
        for (const auto& [id, sector] : sectors_) {
            if (sector.containsPoint(point)) {
                return &sector;
            }
        }
        return nullptr;
    }

    bool isBlocked(sf::Vector2f from, sf::Vector2f to, float radius = 0.3f, bool isJumping = false) const {
        const Sector* startSector = findSectorAt(from);
        
        if (!startSector) {
            return true;
        }

        for (const auto& wall : startSector->getWalls()) {
            sf::Vector2f closest = wall.closestPoint(to);
            sf::Vector2f delta = to - closest;
            float distSquared = delta.x * delta.x + delta.y * delta.y;
            
            if (distSquared < radius * radius) {
                if (wall.isSolid()) {
                    return true;
                }
                if (wall.isPortal()) {
                    Sector* neighbor = wall.getNeighborSector();
                    if (neighbor) {
                        float heightDiff = neighbor->getFloorHeight() - startSector->getFloorHeight();
                        if (heightDiff > MAX_STEP_HEIGHT && !isJumping) {
                            return true;
                        }
                    }
                }
            }
        }
        
        const Sector* endSector = findSectorAt(to);
        if (!endSector) {
            return true;
        }
        
        if (endSector != startSector) {
            float heightDiff = endSector->getFloorHeight() - startSector->getFloorHeight();
            if (heightDiff > MAX_STEP_HEIGHT && !isJumping) {
                return true;
            }

            // Check collisions with walls in the new sector to prevent tunneling
            for (const auto& wall : endSector->getWalls()) {
                sf::Vector2f closest = wall.closestPoint(to);
                sf::Vector2f delta = to - closest;
                float distSquared = delta.x * delta.x + delta.y * delta.y;
                
                if (distSquared < radius * radius) {
                    if (wall.isSolid()) {
                        return true;
                    }
                    if (wall.isPortal()) {
                        Sector* neighbor = wall.getNeighborSector();
                        if (neighbor) {
                            float neighborHeightDiff = neighbor->getFloorHeight() - endSector->getFloorHeight();
                            if (neighborHeightDiff > MAX_STEP_HEIGHT && !isJumping) {
                                return true;
                            }
                        }
                    }
                }
            }
        }
        
        return false;
    }

    void clear() {
        sectors_.clear();
        std::cout << "[SectorMap] Cleared all sectors" << std::endl;
    }

    bool validate() const {
        std::cout << "[SectorMap] Validating map..." << std::endl;

        for (const auto& [id, sector] : sectors_) {
            if (!sector.isValid()) {
                std::cerr << "[SectorMap] ERROR: Invalid sector " << id << std::endl;
                return false;
            }

            // Check portal connections
            for (const auto& wall : sector.getWalls()) {
                if (wall.isPortal()) {
                    Sector* neighbor = wall.getNeighborSector();
                    if (!neighbor) {
                        std::cerr << "[SectorMap] ERROR: Portal with null neighbor in sector " << id << std::endl;
                        return false;
                    }
                }
            }
        }

        std::cout << "[SectorMap] Validation passed (" << sectors_.size() << " sectors)" << std::endl;
        return true;
    }

    void render2D(sf::RenderWindow& window, sf::Vector2f cameraPos, float zoom = 1.0f) const {
        for (const auto& [id, sector] : sectors_) {
            sf::FloatRect bounds = sector.getBounds();
            sf::RectangleShape boundsShape;
            boundsShape.setSize({bounds.size.x, bounds.size.y});
            boundsShape.setPosition({bounds.position.x, bounds.position.y});
            boundsShape.setFillColor(sf::Color(100, 100, 100, 30));
            boundsShape.setOutlineColor(sf::Color(150, 150, 150, 100));
            boundsShape.setOutlineThickness(1.0f);
            window.draw(boundsShape);

            for (const auto& wall : sector.getWalls()) {
                sf::Color wallColor = wall.isSolid() ? sf::Color::Red : sf::Color::Green;
                
                sf::Vertex line[2];
                line[0].position = wall.getStart();
                line[0].color = wallColor;
                line[1].position = wall.getEnd();
                line[1].color = wallColor;
                window.draw(line, 2, sf::PrimitiveType::Lines);

                sf::Vector2f mid = (wall.getStart() + wall.getEnd()) / 2.0f;
                sf::Vector2f normal = wall.getNormal() * 10.0f;
                sf::Vertex normalLine[2];
                normalLine[0].position = mid;
                normalLine[0].color = sf::Color::Yellow;
                normalLine[1].position = mid + normal;
                normalLine[1].color = sf::Color::Yellow;
                window.draw(normalLine, 2, sf::PrimitiveType::Lines);
            }
        }
    }

    sf::FloatRect getBounds() const {
        if (sectors_.empty()) {
            return sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
        }

        bool first = true;
        float minX, maxX, minY, maxY;

        for (const auto& [id, sector] : sectors_) {
            sf::FloatRect bounds = sector.getBounds();
            
            if (first) {
                minX = bounds.position.x;
                maxX = bounds.position.x + bounds.size.x;
                minY = bounds.position.y;
                maxY = bounds.position.y + bounds.size.y;
                first = false;
            } else {
                minX = std::min(minX, bounds.position.x);
                maxX = std::max(maxX, bounds.position.x + bounds.size.x);
                minY = std::min(minY, bounds.position.y);
                maxY = std::max(maxY, bounds.position.y + bounds.size.y);
            }
        }

        return sf::FloatRect({minX, minY}, {maxX - minX, maxY - minY});
    }

private:
    std::unordered_map<int, Sector> sectors_;

    bool lineSegmentIntersectsCircle(const Wall& wall, sf::Vector2f lineStart, 
                                     sf::Vector2f lineEnd, float radius) const {
        sf::Vector2f lineMid = (lineStart + lineEnd) / 2.0f;
        sf::Vector2f closest = wall.closestPoint(lineMid);
        
        sf::Vector2f delta = closest - lineMid;
        float distSquared = delta.x * delta.x + delta.y * delta.y;
        
        return distSquared < (radius * radius);
    }
};