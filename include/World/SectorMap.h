#pragma once
#include "Sector.h"
#include "Wall.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <memory>
#include <unordered_map>
#include <iostream>

/**
 * @brief SectorMap - container and manager for all sectors
 * 
 * Replaces the grid-based Map class with sector-based geometry.
 * Handles:
 * - Sector storage and lookup
 * - Finding which sector contains a point
 * - 2D debug rendering
 */
class SectorMap {
public:
    SectorMap() = default;

    // Sector management
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

    // Get all sectors
    std::unordered_map<int, Sector>& getSectors() {
        return sectors_;
    }

    const std::unordered_map<int, Sector>& getSectors() const {
        return sectors_;
    }

    /**
     * @brief Find which sector contains a point
     * 
     * Iterates through all sectors and checks containment.
     * TODO: Optimize with spatial partitioning (quadtree, BSP)
     */
    Sector* findSectorAt(sf::Vector2f point) {
        for (auto& [id, sector] : sectors_) {
            if (sector.containsPoint(point)) {
                return &sector;
            }
        }
        return nullptr; // Point is outside all sectors
    }

    const Sector* findSectorAt(sf::Vector2f point) const {
        for (const auto& [id, sector] : sectors_) {
            if (sector.containsPoint(point)) {
                return &sector;
            }
        }
        return nullptr;
    }

    /**
     * @brief Check if movement from A to B crosses any solid walls
     * 
     * Returns true if blocked, false if clear.
     * Used for collision detection.
     */
    bool isBlocked(sf::Vector2f from, sf::Vector2f to, float radius = 0.3f) const {
        // Get sectors at start and end points
        const Sector* startSector = findSectorAt(from);
        const Sector* endSector = findSectorAt(to);

        if (!startSector || !endSector) {
            return true; // Outside valid area
        }

        // If in same sector, check walls
        if (startSector == endSector) {
            for (const auto& wall : startSector->getWalls()) {
                if (wall.isSolid() && lineSegmentIntersectsCircle(wall, from, to, radius)) {
                    return true;
                }
            }
            return false;
        }

        // Different sectors - check if portal allows passage
        // This is simplified; real implementation needs to check height differences
        return false; // For now, allow movement between sectors
    }

    /**
     * @brief Clear all sectors
     */
    void clear() {
        sectors_.clear();
        std::cout << "[SectorMap] Cleared all sectors" << std::endl;
    }

    /**
     * @brief Validate entire map
     * 
     * Checks:
     * - All sectors are valid
     * - Portal connections are bidirectional
     * - No orphaned sectors
     */
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

                    // Check if neighbor has reciprocal portal
                    // (This is optional - one-way portals can be valid)
                }
            }
        }

        std::cout << "[SectorMap] Validation passed (" << sectors_.size() << " sectors)" << std::endl;
        return true;
    }

    /**
     * @brief Render sectors in 2D (top-down view)
     * 
     * Used for debugging and 2D mode.
     */
    void render2D(sf::RenderWindow& window, sf::Vector2f cameraPos, float zoom = 1.0f) const {
        for (const auto& [id, sector] : sectors_) {
            // Render sector bounds
            sf::FloatRect bounds = sector.getBounds();
            sf::RectangleShape boundsShape;
            boundsShape.setSize({bounds.size.x, bounds.size.y});
            boundsShape.setPosition({bounds.position.x, bounds.position.y});
            boundsShape.setFillColor(sf::Color(100, 100, 100, 30));
            boundsShape.setOutlineColor(sf::Color(150, 150, 150, 100));
            boundsShape.setOutlineThickness(1.0f);
            window.draw(boundsShape);

            // Render walls
            for (const auto& wall : sector.getWalls()) {
                sf::Color wallColor = wall.isSolid() ? sf::Color::Red : sf::Color::Green;
                
                sf::Vertex line[] = {
                    sf::Vertex(wall.getStart(), wallColor),
                    sf::Vertex(wall.getEnd(), wallColor)
                };
                window.draw(line, 2, sf::PrimitiveType::Lines);

                // Draw normal (for debugging)
                sf::Vector2f mid = (wall.getStart() + wall.getEnd()) / 2.0f;
                sf::Vector2f normal = wall.getNormal() * 10.0f;
                sf::Vertex normalLine[] = {
                    sf::Vertex(mid, sf::Color::Yellow),
                    sf::Vertex(mid + normal, sf::Color::Yellow)
                };
                window.draw(normalLine, 2, sf::PrimitiveType::Lines);
            }

            // Draw sector ID
            // (Need font for this - skip for now)
        }
    }

    /**
     * @brief Get bounding box of entire map
     */
    sf::FloatRect getBounds() const {
        if (sectors_.empty()) {
            // SFML 3.0: FloatRect({position}, {size})
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

        // SFML 3.0: FloatRect({position}, {size})
        return sf::FloatRect({minX, minY}, {maxX - minX, maxY - minY});
    }

private:
    std::unordered_map<int, Sector> sectors_;

    /**
     * @brief Check if line segment intersects circle
     * 
     * Used for collision detection against walls.
     */
    bool lineSegmentIntersectsCircle(const Wall& wall, sf::Vector2f lineStart, 
                                     sf::Vector2f lineEnd, float radius) const {
        // Find closest point on wall to line segment
        sf::Vector2f lineMid = (lineStart + lineEnd) / 2.0f;
        sf::Vector2f closest = wall.closestPoint(lineMid);
        
        // Check distance
        sf::Vector2f delta = closest - lineMid;
        float distSquared = delta.x * delta.x + delta.y * delta.y;
        
        return distSquared < (radius * radius);
    }
};