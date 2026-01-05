#pragma once
#include "Wall.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>

/**
 * @brief Sector class for Doom-style engine
 * 
 * A sector is a polygonal region defined by walls.
 * Each sector has:
 * - Floor height and ceiling height
 * - Floor and ceiling textures
 * - Light level
 * - List of walls
 * 
 * Portals between sectors allow visibility and movement.
 */
class Sector {
public:
    // Constructors
    Sector() 
        : id_(-1)
        , floorHeight_(0.0f)
        , ceilingHeight_(2.5f)
        , lightLevel_(255)
        , floorTexture_("")
        , ceilingTexture_("")
    {}

    Sector(int id, float floorHeight, float ceilingHeight)
        : id_(id)
        , floorHeight_(floorHeight)
        , ceilingHeight_(ceilingHeight)
        , lightLevel_(255)
        , floorTexture_("")
        , ceilingTexture_("")
    {}

    // ID
    int getId() const { return id_; }
    void setId(int id) { id_ = id; }

    // Heights
    float getFloorHeight() const { return floorHeight_; }
    float getCeilingHeight() const { return ceilingHeight_; }
    void setFloorHeight(float height) { floorHeight_ = height; }
    void setCeilingHeight(float height) { ceilingHeight_ = height; }
    
    float getHeight() const { return ceilingHeight_ - floorHeight_; }

    // Textures
    const std::string& getFloorTexture() const { return floorTexture_; }
    const std::string& getCeilingTexture() const { return ceilingTexture_; }
    void setFloorTexture(const std::string& tex) { floorTexture_ = tex; }
    void setCeilingTexture(const std::string& tex) { ceilingTexture_ = tex; }

    // Lighting
    uint8_t getLightLevel() const { return lightLevel_; }
    void setLightLevel(uint8_t light) { lightLevel_ = light; }
    
    sf::Color getLightColor() const {
        return sf::Color(lightLevel_, lightLevel_, lightLevel_);
    }

    // Walls
    std::vector<Wall>& getWalls() { return walls_; }
    const std::vector<Wall>& getWalls() const { return walls_; }
    
    void addWall(const Wall& wall) { walls_.push_back(wall); }
    void clearWalls() { walls_.clear(); }
    
    size_t getWallCount() const { return walls_.size(); }
    Wall& getWall(size_t index) { return walls_[index]; }
    const Wall& getWall(size_t index) const { return walls_[index]; }

    // Geometry utilities
    
    /**
     * @brief Check if a point is inside this sector
     * 
     * Uses ray casting algorithm to determine if point is inside polygon.
     * Note: This assumes the sector is a valid polygon (no self-intersections).
     */
    bool containsPoint(sf::Vector2f point) const {
        if (walls_.empty()) return false;

        int intersections = 0;
        size_t wallCount = walls_.size();

        for (size_t i = 0; i < wallCount; ++i) {
            const Wall& wall = walls_[i];
            sf::Vector2f p1 = wall.getStart();
            sf::Vector2f p2 = wall.getEnd();

            // Ray casting: shoot ray to the right from point
            if ((p1.y > point.y) != (p2.y > point.y)) {
                float slope = (p2.x - p1.x) / (p2.y - p1.y);
                float intersectX = p1.x + slope * (point.y - p1.y);
                
                if (point.x < intersectX) {
                    intersections++;
                }
            }
        }

        // Odd number of intersections = inside
        return (intersections % 2) == 1;
    }

    /**
     * @brief Get bounding box of sector
     */
    sf::FloatRect getBounds() const {
        if (walls_.empty()) {
            // SFML 3.0: FloatRect({position}, {size})
            return sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
        }

        float minX = walls_[0].getStart().x;
        float maxX = minX;
        float minY = walls_[0].getStart().y;
        float maxY = minY;

        for (const auto& wall : walls_) {
            sf::Vector2f start = wall.getStart();
            sf::Vector2f end = wall.getEnd();

            minX = std::min({minX, start.x, end.x});
            maxX = std::max({maxX, start.x, end.x});
            minY = std::min({minY, start.y, end.y});
            maxY = std::max({maxY, start.y, end.y});
        }

        // SFML 3.0: FloatRect({position}, {size})
        return sf::FloatRect({minX, minY}, {maxX - minX, maxY - minY});
    }

    /**
     * @brief Get center point of sector
     */
    sf::Vector2f getCenter() const {
        sf::FloatRect bounds = getBounds();
        return sf::Vector2f(
            bounds.position.x + bounds.size.x / 2.0f,
            bounds.position.y + bounds.size.y / 2.0f
        );
    }

    /**
     * @brief Validate sector geometry
     * 
     * Checks:
     * - At least 3 walls
     * - Walls form closed loop
     * - No degenerate walls (zero length)
     */
    bool isValid() const {
        if (walls_.size() < 3) return false;
        
        // Check if walls form closed loop
        if (walls_.size() > 0) {
            if (walls_.back().getEnd() != walls_.front().getStart()) {
                return false; // Not closed
            }
        }

        // Check for degenerate walls
        for (const auto& wall : walls_) {
            if (wall.getLength() < 0.01f) {
                return false; // Degenerate wall
            }
        }

        return true;
    }

    // Special properties
    enum SectorFlags {
        FLAG_NONE = 0,
        FLAG_DAMAGE = 1 << 0,        // Damages player (lava, acid)
        FLAG_SECRET = 1 << 1,        // Secret area
        FLAG_WATER = 1 << 2,         // Water sector (affects movement)
        FLAG_OUTDOOR = 1 << 3,       // Outdoor (sky ceiling)
    };

    bool hasFlag(SectorFlags flag) const { return (flags_ & flag) != 0; }
    void setFlag(SectorFlags flag) { flags_ |= flag; }
    void clearFlag(SectorFlags flag) { flags_ &= ~flag; }
    uint32_t getFlags() const { return flags_; }
    void setFlags(uint32_t flags) { flags_ = flags; }

private:
    // Identification
    int id_;

    // Heights (in world units, e.g., meters)
    float floorHeight_;
    float ceilingHeight_;

    // Textures
    std::string floorTexture_;
    std::string ceilingTexture_;

    // Lighting (0-255)
    uint8_t lightLevel_;

    // Walls defining the sector
    std::vector<Wall> walls_;

    // Special properties
    uint32_t flags_ = 0;
};