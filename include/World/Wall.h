#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <cmath>

// Forward declaration
class Sector;

/**
 * @brief Wall class for sector-based engine
 * 
 * A wall is a line segment between two points.
 * Walls can be solid or portals (if neighborSector is set).
 * 
 * Portal walls show the neighboring sector through them,
 * and render upper/lower textures based on height differences.
 */
class Wall {
public:
    // Constructors
    Wall() = default;
    
    Wall(sf::Vector2f start, sf::Vector2f end)
        : start_(start)
        , end_(end)
        , neighborSector_(nullptr)
        , flags_(0)
    {}

    // Geometry
    sf::Vector2f getStart() const { return start_; }
    sf::Vector2f getEnd() const { return end_; }
    void setStart(sf::Vector2f pos) { start_ = pos; }
    void setEnd(sf::Vector2f pos) { end_ = pos; }

    // Portal connection
    Sector* getNeighborSector() const { return neighborSector_; }
    void setNeighborSector(Sector* sector) { neighborSector_ = sector; }
    bool isPortal() const { return neighborSector_ != nullptr; }
    bool isSolid() const { return neighborSector_ == nullptr; }

    // Textures
    const std::string& getUpperTexture() const { return upperTexture_; }
    const std::string& getMiddleTexture() const { return middleTexture_; }
    const std::string& getLowerTexture() const { return lowerTexture_; }
    
    void setUpperTexture(const std::string& tex) { upperTexture_ = tex; }
    void setMiddleTexture(const std::string& tex) { middleTexture_ = tex; }
    void setLowerTexture(const std::string& tex) { lowerTexture_ = tex; }

    // Geometry utilities
    float getLength() const {
        sf::Vector2f delta = end_ - start_;
        return std::sqrt(delta.x * delta.x + delta.y * delta.y);
    }

    sf::Vector2f getNormal() const {
        sf::Vector2f delta = end_ - start_;
        // Perpendicular vector (right-hand side)
        return sf::Vector2f(-delta.y, delta.x) / getLength();
    }

    // Check which side of the wall a point is on
    // Returns: positive = right side, negative = left side, 0 = on line
    float getSide(sf::Vector2f point) const {
        sf::Vector2f delta = end_ - start_;
        return (point.x - start_.x) * delta.y - (point.y - start_.y) * delta.x;
    }

    // Closest point on wall segment to given point
    sf::Vector2f closestPoint(sf::Vector2f point) const {
        sf::Vector2f delta = end_ - start_;
        float lengthSquared = delta.x * delta.x + delta.y * delta.y;
        
        if (lengthSquared < 0.0001f) {
            return start_; // Degenerate wall
        }

        // Project point onto line
        float t = ((point.x - start_.x) * delta.x + (point.y - start_.y) * delta.y) / lengthSquared;
        t = std::clamp(t, 0.0f, 1.0f);

        return start_ + delta * t;
    }

    // Flags for special wall properties
    enum WallFlags {
        FLAG_NONE = 0,
        FLAG_BLOCKING = 1 << 0,      // Blocks player movement
        FLAG_TRANSPARENT = 1 << 1,    // See-through (glass)
        FLAG_TWO_SIDED = 1 << 2,      // Visible from both sides
        FLAG_DOOR = 1 << 3,           // Part of door mechanism
    };

    bool hasFlag(WallFlags flag) const { return (flags_ & flag) != 0; }
    void setFlag(WallFlags flag) { flags_ |= flag; }
    void clearFlag(WallFlags flag) { flags_ &= ~flag; }
    uint32_t getFlags() const { return flags_; }
    void setFlags(uint32_t flags) { flags_ = flags; }

private:
    // Geometry
    sf::Vector2f start_;
    sf::Vector2f end_;

    // Portal connection (nullptr = solid wall)
    Sector* neighborSector_;

    // Textures
    std::string upperTexture_;  // Above portal opening
    std::string middleTexture_; // Solid wall or middle of portal
    std::string lowerTexture_;  // Below portal opening

    // Flags
    uint32_t flags_;
};