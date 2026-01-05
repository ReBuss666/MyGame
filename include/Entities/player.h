#pragma once
#include <SFML/Graphics.hpp>
#include "../Utils/settings.h"
#include <iostream>
#include <cmath>
#include <algorithm>

/**
 * @brief Player class - updated for Sector Engine
 * 
 * Changes:
 * - Removed dependency on old Map class
 * - Added setPosition() method
 * - Added updateSimple() for movement without collision detection
 * - Collision detection will be handled by SectorMap in the future
 */
class Player {
public:
    Player(sf::Vector2f position)
        : viewAngle_(0.0f)
        , mouseSensitivity_(MOUSE_SENSITIVITY)
        , lastMouseX_(0.f)
        , firstMouse_(true)
    {
        shape_.setSize({PLAYER_WIDTH, PLAYER_HEIGHT}); 
        shape_.setOrigin({PLAYER_WIDTH / 2.f, PLAYER_HEIGHT / 2.f});
        shape_.setPosition(position);
        shape_.setFillColor(sf::Color(0, 255, 0));

        velocity_ = sf::Vector2f(0.f, 0.f);
        maxSpeed_ = PLAYER_SPEED;     
        acceleration_ = PLAYER_ACCELERATION;
        friction_ = PLAYER_FRICTION; 
        
        std::cout << "[Player] Created at (" << position.x << ", " << position.y << ")" << std::endl;
    }

    /**
     * @brief Simple update without collision detection
     * 
     * Use this for temporary testing until proper sector collision is implemented.
     * 
     * @param deltaTime Time since last frame
     * @param mode3D If true, movement is relative to view direction
     */
    void updateSimple(float deltaTime, bool mode3D = false) {
        // 1. Input handling
        sf::Vector2f inputDir{0.f, 0.f};

        if (mode3D) {
            // 3D Mode: WASD movement relative to view direction
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
                inputDir.x += std::cos(viewAngle_);
                inputDir.y += std::sin(viewAngle_);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
                inputDir.x -= std::cos(viewAngle_);
                inputDir.y -= std::sin(viewAngle_);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                inputDir.x += std::cos(viewAngle_ - M_PI / 2.f);
                inputDir.y += std::sin(viewAngle_ - M_PI / 2.f);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                inputDir.x += std::cos(viewAngle_ + M_PI / 2.f);
                inputDir.y += std::sin(viewAngle_ + M_PI / 2.f);
            }
        } else {
            // 2D Mode: Axis-aligned movement
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) inputDir.y -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) inputDir.y += 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) inputDir.x -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) inputDir.x += 1.f;
        }

        // 2. Normalize diagonal movement
        if (inputDir.x != 0.f || inputDir.y != 0.f) {
            float length = std::sqrt(inputDir.x * inputDir.x + inputDir.y * inputDir.y);
            inputDir /= length; 
            velocity_ += inputDir * acceleration_ * deltaTime;
        }

        // 3. Apply friction
        velocity_ -= velocity_ * friction_ * deltaTime;

        // 4. Limit max speed
        float currentSpeed = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y);
        if (currentSpeed > maxSpeed_) {
            velocity_ = (velocity_ / currentSpeed) * maxSpeed_;
        }

        // 5. Move player (no collision check)
        shape_.move({velocity_.x * deltaTime, velocity_.y * deltaTime});
    }

    void handleMouseLook(float mouseX) {
        if (firstMouse_) {
            lastMouseX_ = mouseX;
            firstMouse_ = false;
            return;
        }
        
        float deltaX = mouseX - lastMouseX_;
        lastMouseX_ = mouseX;

        // Rotate camera
        viewAngle_ += deltaX * mouseSensitivity_;

        // Keep angle in [0, 2PI]
        while (viewAngle_ < 0) viewAngle_ += 2.0f * M_PI;
        while (viewAngle_ >= 2.0f * M_PI) viewAngle_ -= 2.0f * M_PI;
    }

    void handleKeyboardRotation(float deltaTime) {
        float rotationSpeed = KEYBOARD_ROTATION_SPEED;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            viewAngle_ -= rotationSpeed * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            viewAngle_ += rotationSpeed * deltaTime;
        }

        // Normalize angle
        while (viewAngle_ < 0) viewAngle_ += 2.0f * M_PI;
        while (viewAngle_ >= 2.0f * M_PI) viewAngle_ -= 2.0f * M_PI;
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape_);
    }

    // Getters
    sf::Vector2f getPosition() const { return shape_.getPosition(); }
    sf::Vector2f getVelocity() const { return velocity_; }
    float getViewAngle() const { return viewAngle_; }
    
    // Setters
    void setPosition(sf::Vector2f pos) { 
        shape_.setPosition(pos); 
    }
    
    void setViewAngle(float angle) { viewAngle_ = angle; }
    void setMouseSensitivity(float sensitivity) { mouseSensitivity_ = sensitivity; }
    void resetMouseTracking() { firstMouse_ = true; }

private:
    sf::Vector2f velocity_;
    sf::RectangleShape shape_;
    float maxSpeed_;
    float acceleration_;    
    float friction_;

    // FPS camera data
    float viewAngle_;         // View angle in radians (0 = right, PI/2 = down)
    float mouseSensitivity_;  // Mouse sensitivity
    float lastMouseX_;        // Last mouse X position
    bool firstMouse_;         // First mouse movement flag
};