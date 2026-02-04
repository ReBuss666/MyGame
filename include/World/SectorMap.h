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