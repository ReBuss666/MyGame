#pragma once
#include <vector>
#include <iostream>
#include <SFML/Graphics.hpp>

/**
 * @brief Optimized tile-based map with batch rendering
 * 
 * Uses sf::VertexArray for efficient rendering (1 draw call instead of 500+)
 * Compatible with SFML 3.0
 */
class Map {
public:
    Map() : tileSize_(64.f) {
        // Initialize grid
        grid_ = {
            {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
            {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,1,0,0,1,1,1,0,0,1,0,1,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
            {1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1},
            {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}
        };

        mapHeight_ = static_cast<int>(grid_.size());
        mapWidth_ = mapHeight_ > 0 ? static_cast<int>(grid_[0].size()) : 0;
        
        // Build vertex array once at construction
        buildVertexArray();
        
        std::cout << "[Map] Initialized: " << mapWidth_ << "x" << mapHeight_ 
                  << " (" << wallVertices_.getVertexCount() << " vertices)" << std::endl;
    }

    bool isWall(float x, float y) const {
        int gridX = static_cast<int>(x / tileSize_);
        int gridY = static_cast<int>(y / tileSize_);
        if (gridX < 0 || gridX >= mapWidth_ || gridY < 0 || gridY >= mapHeight_) return true;
        return grid_[gridY][gridX] != 0;
    }

    void render(sf::RenderWindow& window) {
        // Single draw call for all walls - HUGE performance improvement!
        window.draw(wallVertices_);
    }

    float getTileSize() const { return tileSize_; }
    int getWidth() const { return mapWidth_; }
    int getHeight() const { return mapHeight_; }

private:
    float tileSize_;
    int mapWidth_;
    int mapHeight_;
    std::vector<std::vector<int>> grid_;
    sf::VertexArray wallVertices_;

    /**
     * @brief Build vertex array for all walls
     * 
     * Called once at construction. Creates 6 vertices (2 triangles) per wall tile.
     * SFML 3.0 compatible - uses sf::Vector2f with proper initialization.
     */
    void buildVertexArray() {
        // Count walls first
        int wallCount = 0;
        for (int y = 0; y < mapHeight_; ++y) {
            for (int x = 0; x < mapWidth_; ++x) {
                if (grid_[y][x] != 0) {
                    wallCount++;
                }
            }
        }

        // Pre-allocate vertex array (6 vertices per wall = 2 triangles)
        wallVertices_.setPrimitiveType(sf::PrimitiveType::Triangles);
        wallVertices_.resize(wallCount * 6);

        // Fill vertex array
        int vertexIndex = 0;
        const float padding = 2.f; // Visual gap between tiles
        const sf::Color wallColor = sf::Color::Red;

        for (int y = 0; y < mapHeight_; ++y) {
            for (int x = 0; x < mapWidth_; ++x) {
                if (grid_[y][x] != 0) {
                    // Calculate tile position (with padding)
                    float left = x * tileSize_;
                    float top = y * tileSize_;
                    float right = left + tileSize_ - padding;
                    float bottom = top + tileSize_ - padding;

                    // First triangle (top-left, top-right, bottom-left)
                    wallVertices_[vertexIndex + 0].position = sf::Vector2f{left, top};
                    wallVertices_[vertexIndex + 0].color = wallColor;
                    
                    wallVertices_[vertexIndex + 1].position = sf::Vector2f{right, top};
                    wallVertices_[vertexIndex + 1].color = wallColor;
                    
                    wallVertices_[vertexIndex + 2].position = sf::Vector2f{left, bottom};
                    wallVertices_[vertexIndex + 2].color = wallColor;

                    // Second triangle (bottom-left, top-right, bottom-right)
                    wallVertices_[vertexIndex + 3].position = sf::Vector2f{left, bottom};
                    wallVertices_[vertexIndex + 3].color = wallColor;
                    
                    wallVertices_[vertexIndex + 4].position = sf::Vector2f{right, top};
                    wallVertices_[vertexIndex + 4].color = wallColor;
                    
                    wallVertices_[vertexIndex + 5].position = sf::Vector2f{right, bottom};
                    wallVertices_[vertexIndex + 5].color = wallColor;

                    vertexIndex += 6;
                }
            }
        }
    }

    void rebuildVertexArray() {
        buildVertexArray();
    }
};