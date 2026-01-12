#pragma once

#include <SFML/Graphics.hpp>
#include <vector>
#include <array>
#include <random>

class FireEffect {
public:
    FireEffect(int width, int height, int pixelSize);
    ~FireEffect() = default;

    void update();
    void render(sf::RenderWindow& window);
    void triggerFlash();

    void enableFuel();
    void disableFuel();

    bool isFlashActive() const;
    void resetFlash();

private:
    int width_, height_, pixelSize_;

    std::vector<int> firePixels_;
    std::array<sf::Color, 37> palette_;

    sf::VertexArray vertices_;

    bool flashActive_;
    int flashCounter_;
    int flashDuration_;

    bool fuelEnabled_;

    std::mt19937 rng_;
    std::uniform_int_distribution<int> decayDist_;
    std::uniform_int_distribution<int> driftDist_;
    std::uniform_int_distribution<int> flashDist_;

    void initPalette();
    void fuel();
    void doFire(int x, int y);
    void updateVertices();
    int getIndex(int x, int y) const;
};