#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <cmath>

class Sector;

class Wall {
public:
    Wall() = default;
    
    Wall(sf::Vector2f start, sf::Vector2f end)
        : start_(start)
        , end_(end)
        , neighborSector_(nullptr)
        , flags_(0)
    {}

    sf::Vector2f getStart() const { return start_; }
    sf::Vector2f getEnd() const { return end_; }
    void setStart(sf::Vector2f pos) { start_ = pos; }
    void setEnd(sf::Vector2f pos) { end_ = pos; }

    Sector* getNeighborSector() const { return neighborSector_; }
    void setNeighborSector(Sector* sector) { neighborSector_ = sector; }
    bool isPortal() const { return neighborSector_ != nullptr; }
    bool isSolid() const { return neighborSector_ == nullptr; }

    const std::string& getUpperTexture() const { return upperTexture_; }
    const std::string& getMiddleTexture() const { return middleTexture_; }
    const std::string& getLowerTexture() const { return lowerTexture_; }
    
    void setUpperTexture(const std::string& tex) { upperTexture_ = tex; }
    void setMiddleTexture(const std::string& tex) { middleTexture_ = tex; }
    void setLowerTexture(const std::string& tex) { lowerTexture_ = tex; }

    float getLength() const {
        sf::Vector2f delta = end_ - start_;
        return std::sqrt(delta.x * delta.x + delta.y * delta.y);
    }

    sf::Vector2f getNormal() const {
        sf::Vector2f delta = end_ - start_;
        return sf::Vector2f(-delta.y, delta.x) / getLength();
    }

    float getSide(sf::Vector2f point) const {
        sf::Vector2f delta = end_ - start_;
        return (point.x - start_.x) * delta.y - (point.y - start_.y) * delta.x;
    }

    sf::Vector2f closestPoint(sf::Vector2f point) const {
        sf::Vector2f delta = end_ - start_;
        float lengthSquared = delta.x * delta.x + delta.y * delta.y;
        
        if (lengthSquared < 0.0001f) {
            return start_;
        }

        float t = ((point.x - start_.x) * delta.x + (point.y - start_.y) * delta.y) / lengthSquared;
        if (t < 0.0f) t = 0.0f;
        if (t > 1.0f) t = 1.0f;

        return start_ + delta * t;
    }

    enum WallFlags {
        FLAG_NONE = 0,
        FLAG_BLOCKING = 1 << 0,
        FLAG_TRANSPARENT = 1 << 1,
        FLAG_TWO_SIDED = 1 << 2,
        FLAG_DOOR = 1 << 3,
    };

    bool hasFlag(WallFlags flag) const { return (flags_ & flag) != 0; }
    void setFlag(WallFlags flag) { flags_ |= flag; }
    void clearFlag(WallFlags flag) { flags_ &= ~flag; }
    uint32_t getFlags() const { return flags_; }
    void setFlags(uint32_t flags) { flags_ = flags; }

    sf::Color getColor() const { return color_; }
    void setColor(sf::Color c) { color_ = c; }

    int getNextSectorId() const { return nextSectorId_; }
    void setNextSectorId(int id) { nextSectorId_ = id; }

private:
    sf::Vector2f start_;
    sf::Vector2f end_;
    Sector* neighborSector_;
    std::string upperTexture_;
    std::string middleTexture_;
    std::string lowerTexture_;
    uint32_t flags_;
    
    sf::Color color_ = sf::Color::White;
    int nextSectorId_ = -1;
};