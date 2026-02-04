#pragma once
#include "Wall.h"
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <memory>
#include <algorithm>

class Sector {
public:
    Sector() 
        : id_(-1)
        , floorHeight_(0.0f)
        , ceilingHeight_(2.5f)
        , floorTexture_("")
        , ceilingTexture_("")
        , lightLevel_(255)
    {}

    Sector(int id, float floorHeight, float ceilingHeight)
        : id_(id)
        , floorHeight_(floorHeight)
        , ceilingHeight_(ceilingHeight)
        , floorTexture_("")
        , ceilingTexture_("")
        , lightLevel_(255)
    {}

    int getId() const { return id_; }
    void setId(int id) { id_ = id; }

    float getFloorHeight() const { return floorHeight_; }
    float getCeilingHeight() const { return ceilingHeight_; }
    void setFloorHeight(float height) { floorHeight_ = height; }
    void setCeilingHeight(float height) { ceilingHeight_ = height; }
    
    float getHeight() const { return ceilingHeight_ - floorHeight_; }

    const std::string& getFloorTexture() const { return floorTexture_; }
    const std::string& getCeilingTexture() const { return ceilingTexture_; }
    void setFloorTexture(const std::string& tex) { floorTexture_ = tex; }
    void setCeilingTexture(const std::string& tex) { ceilingTexture_ = tex; }

    uint8_t getLightLevel() const { return lightLevel_; }
    void setLightLevel(uint8_t light) { lightLevel_ = light; }
    
    sf::Color getLightColor() const {
        return sf::Color(lightLevel_, lightLevel_, lightLevel_);
    }

    sf::Color getFloorColor() const { return floorColor_; }
    void setFloorColor(sf::Color c) { floorColor_ = c; }
    
    sf::Color getCeilingColor() const { return ceilingColor_; }
    void setCeilingColor(sf::Color c) { ceilingColor_ = c; }

    std::vector<Wall>& getWalls() { return walls_; }
    const std::vector<Wall>& getWalls() const { return walls_; }
    
    void addWall(const Wall& wall) { walls_.push_back(wall); }
    void clearWalls() { walls_.clear(); }
    
    size_t getWallCount() const { return walls_.size(); }
    Wall& getWall(size_t index) { return walls_[index]; }
    const Wall& getWall(size_t index) const { return walls_[index]; }

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

        return (intersections % 2) == 1;
    }

    sf::FloatRect getBounds() const {
        if (walls_.empty()) {
            return sf::FloatRect({0.0f, 0.0f}, {0.0f, 0.0f});
        }

        float minX = walls_[0].getStart().x;
        float maxX = minX;
        float minY = walls_[0].getStart().y;
        float maxY = minY;

        for (const auto& wall : walls_) {
            sf::Vector2f start = wall.getStart();
            sf::Vector2f end = wall.getEnd();

            minX = std::min(minX, std::min(start.x, end.x));
            maxX = std::max(maxX, std::max(start.x, end.x));
            minY = std::min(minY, std::min(start.y, end.y));
            maxY = std::max(maxY, std::max(start.y, end.y));
        }

        return sf::FloatRect({minX, minY}, {maxX - minX, maxY - minY});
    }

    sf::Vector2f getCenter() const {
        sf::FloatRect bounds = getBounds();
        return sf::Vector2f(
            bounds.position.x + bounds.size.x / 2.0f,
            bounds.position.y + bounds.size.y / 2.0f
        );
    }

    bool isValid() const {
        if (walls_.size() < 3) return false;
        
        if (walls_.size() > 0) {
            if (walls_.back().getEnd() != walls_.front().getStart()) {
                return false;
            }
        }

        for (const auto& wall : walls_) {
            if (wall.getLength() < 0.01f) {
                return false;
            }
        }

        return true;
    }

    enum SectorFlags {
        FLAG_NONE = 0,
        FLAG_DAMAGE = 1 << 0,
        FLAG_SECRET = 1 << 1,
        FLAG_WATER = 1 << 2,
        FLAG_OUTDOOR = 1 << 3,
    };

    bool hasFlag(SectorFlags flag) const { return (flags_ & flag) != 0; }
    void setFlag(SectorFlags flag) { flags_ |= flag; }
    void clearFlag(SectorFlags flag) { flags_ &= ~flag; }
    uint32_t getFlags() const { return flags_; }
    void setFlags(uint32_t flags) { flags_ = flags; }

private:
    int id_;
    float floorHeight_;
    float ceilingHeight_;
    std::string floorTexture_;
    std::string ceilingTexture_;
    uint8_t lightLevel_;
    std::vector<Wall> walls_;
    uint32_t flags_ = 0;
    
    sf::Color floorColor_ = sf::Color(100, 100, 100);
    sf::Color ceilingColor_ = sf::Color(50, 50, 50);
};