#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <array>
#include <random>

/**
 * @brief Optimized fire particle effect with modern C++ random
 * 
 * Optimizations:
 * - Uses std::mt19937 instead of deprecated rand()
 * - Skips updating transparent pixels
 * - Pre-allocated distributions for better performance
 * 
 * SFML 3.0 compatible
 */
class FireEffect {
public:
    FireEffect(int width, int height, int pixelSize);
    ~FireEffect() = default;

    // Main methods
    void update();
    void render(sf::RenderWindow& window);
    void triggerFlash();

    // Fuel control
    void enableFuel();
    void disableFuel();

    // State control
    bool isFlashActive() const;
    void resetFlash();

private:
    // Sizes
    int width_, height_, pixelSize_;

    // Fire data
    std::vector<int> firePixels_;
    std::array<sf::Color, 37> palette_;

    // Vertex array for batching (6 vertices per pixel = 2 triangles)
    sf::VertexArray vertices_;

    // Flash state
    bool flashActive_;
    int flashCounter_;
    int flashDuration_;

    // Fuel state
    bool fuelEnabled_;

    // Modern C++ random number generation
    std::mt19937 rng_;
    std::uniform_int_distribution<int> decayDist_;
    std::uniform_int_distribution<int> driftDist_;
    std::uniform_int_distribution<int> flashDist_;

    // Private methods
    void initPalette();
    void fuel();
    void doFire(int x, int y);
    void updateVertices();
    int getIndex(int x, int y) const;
};