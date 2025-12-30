#pragma once
#include <SFML/Graphics.hpp>
#include "../World/map.h"
#include "../Utils/settings.h"
#include <iostream>
#include <cmath>
#include <algorithm>

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

    void update(float deltaTime, const Map& gameMap, bool mode3D = false) {
        // 1. Input handling
        sf::Vector2f inputDir{0.f, 0.f};

        if (mode3D) {
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
            // 2D Mode movement (axis-aligned)
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

        // 5. Collision detection and movement
        sf::Vector2f oldPos = shape_.getPosition();
        sf::Vector2f nextPos = oldPos + (velocity_ * deltaTime);

        // --- X Axis ---
        sf::FloatRect nextBoundsX = shape_.getGlobalBounds();
        nextBoundsX.position.x = nextPos.x - shape_.getOrigin().x;
        nextBoundsX.position.y = oldPos.y - shape_.getOrigin().y;

        if (!checkCollision(nextBoundsX, gameMap)) {
            shape_.move({velocity_.x * deltaTime, 0.f});
        } else {
            velocity_.x = 0.f;
        }

        // --- Y Axis ---
        sf::Vector2f currentPosAfterX = shape_.getPosition();
        sf::FloatRect nextBoundsY = shape_.getGlobalBounds();
        
        nextBoundsY.position.x = currentPosAfterX.x - shape_.getOrigin().x;
        nextBoundsY.position.y = nextPos.y - shape_.getOrigin().y;

        if (!checkCollision(nextBoundsY, gameMap)) {
            shape_.move({0.f, velocity_.y * deltaTime});
        } else {
            velocity_.y = 0.f;
        }
    }

    // mouse look for 3D mode
    void handleMouseLook(float mouseX) {
        if (firstMouse_) {
            lastMouseX_ = mouseX;
            firstMouse_ = false;
            return;
        }
        float deltaX = mouseX - lastMouseX_;
        lastMouseX_ = mouseX;

        // camera rotation
        viewAngle_ += deltaX * mouseSensitivity_;

        // keep angle in [0, 2PI]
        while (viewAngle_ < 0) viewAngle_ += 2 * M_PI;
        while (viewAngle_ >= 2 * M_PI) viewAngle_ -= 2 * M_PI;
    }

    void handleKeyboardRotation(float deltaTime) {
        float rotationSpeed = KEYBOARD_ROTATION_SPEED;

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            viewAngle_ -= rotationSpeed * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            viewAngle_ += rotationSpeed * deltaTime;
        }

        // Нормализуем угол
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
    void setViewAngle(float angle) { viewAngle_ = angle; }
    void setMouseSensitivity(float sensitivity) { mouseSensitivity_ = sensitivity; }
    void resetMouseTracking() { firstMouse_ = true; }


private:
    sf::Vector2f velocity_;
    sf::RectangleShape shape_;
    float maxSpeed_;
    float acceleration_;    
    float friction_;

    // FPS данные
    float viewAngle_;         // Угол взгляда в радианах (0 = вправо, PI/2 = вниз)
    float mouseSensitivity_;  // Чувствительность мыши
    float lastMouseX_;        // Последняя позиция мыши X
    bool firstMouse_;         // Первое движение мыши

    bool checkCollision(const sf::FloatRect& bounds, const Map& map) {
        float left = bounds.position.x;
        float top = bounds.position.y;
        float width = bounds.size.x;
        float height = bounds.size.y;

        // Test all 4 corners
        if (map.isWall(left + COLLISION_BUFFER, top + COLLISION_BUFFER)) return true;
        if (map.isWall(left + width - COLLISION_BUFFER, top + COLLISION_BUFFER)) return true;
        if (map.isWall(left + COLLISION_BUFFER, top + height - COLLISION_BUFFER)) return true;
        if (map.isWall(left + width - COLLISION_BUFFER, top + height - COLLISION_BUFFER)) return true;

        return false;
    }
};