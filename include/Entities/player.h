#pragma once
#include <SFML/Graphics.hpp>
#include "../Utils/settings.h"
#include <iostream>
#include <cmath>
#include <algorithm>

class Player {
public:
    Player(sf::Vector2f position)
        : viewAngle_(0.0f)
        , pitchAngle_(0.0f)           // Вертикальный угол взгляда
        , mouseSensitivity_(MOUSE_SENSITIVITY)
        , lastMouseX_(0.f)
        , lastMouseY_(0.f)            // Для вертикального обзора
        , firstMouse_(true)
        // Vertical movement
        , verticalPos_(0.0f)          // Height above current floor
        , verticalVelocity_(0.0f)     // Vertical speed
        , isGrounded_(true)           // On ground?
        , isJumping_(false)           // Currently jumping?
        // Sprint
        , isSprinting_(false)
        // Crouch
        , isCrouching_(false)
        , currentEyeHeight_(PLAYER_STAND_HEIGHT)
        , targetEyeHeight_(PLAYER_STAND_HEIGHT)
        // Head bobbing
        , bobPhase_(0.0f)
        , bobVertical_(0.0f)
        , bobHorizontal_(0.0f)
        , isMoving_(false)
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

    void updateVertical(float deltaTime, float floorHeight) {
        // Apply gravity
        if (!isGrounded_) {
            verticalVelocity_ -= PLAYER_GRAVITY * deltaTime;
            
            // Clamp fall speed
            if (verticalVelocity_ < -PLAYER_MAX_FALL_SPEED) {
                verticalVelocity_ = -PLAYER_MAX_FALL_SPEED;
            }
        }

        // Update vertical position
        verticalPos_ += verticalVelocity_ * deltaTime;

        // Check ground collision
        if (verticalPos_ <= floorHeight) {
            verticalPos_ = floorHeight;
            verticalVelocity_ = 0.0f;
            isGrounded_ = true;
            isJumping_ = false;
        } else {
            isGrounded_ = false;
        }

        // Smooth eye height transition (crouch/stand)
        if (currentEyeHeight_ != targetEyeHeight_) {
            float diff = targetEyeHeight_ - currentEyeHeight_;
            float step = PLAYER_CROUCH_TRANSITION * deltaTime;
            
            if (std::abs(diff) <= step) {
                currentEyeHeight_ = targetEyeHeight_;
            } else {
                currentEyeHeight_ += (diff > 0 ? step : -step);
            }
        }
    }

    void updateHeadBob(float deltaTime, bool isMoving) {
        isMoving_ = isMoving;
        
        if (isMoving && isGrounded_) {
            // Determine bobbing frequency based on state
            float frequency = BOB_FREQUENCY_WALK;
            float amplitudeMultiplier = 1.0f;
            
            if (isSprinting_) {
                frequency = BOB_FREQUENCY_SPRINT;
                amplitudeMultiplier = BOB_SPRINT_MULTIPLIER;
            } else if (isCrouching_) {
                frequency = BOB_FREQUENCY_CROUCH;
                amplitudeMultiplier = BOB_CROUCH_MULTIPLIER;
            }
            
            // Advance bob phase
            bobPhase_ += frequency * deltaTime * 2.0f * M_PI;
            
            // Keep phase in [0, 2PI]
            while (bobPhase_ >= 2.0f * M_PI) {
                bobPhase_ -= 2.0f * M_PI;
            }
            
            // Calculate vertical bob (up/down) - основной эффект
            // Используем abs(sin) для эффекта "шага" - камера опускается при каждом шаге
            bobVertical_ = std::abs(std::sin(bobPhase_)) * BOB_AMPLITUDE_VERTICAL * amplitudeMultiplier;
            
            // Calculate horizontal bob (sway left/right) - дополнительный эффект
            // Горизонтальное покачивание с половинной частотой для более естественного эффекта
            bobHorizontal_ = std::sin(bobPhase_ * 0.5f) * BOB_AMPLITUDE_HORIZONTAL * amplitudeMultiplier;
        } else {
            // Плавное затухание покачивания когда останавливаемся
            bobVertical_ *= (1.0f - 10.0f * deltaTime);
            bobHorizontal_ *= (1.0f - 10.0f * deltaTime);
            
            // Сброс если очень маленькое значение
            if (std::abs(bobVertical_) < 0.001f) bobVertical_ = 0.0f;
            if (std::abs(bobHorizontal_) < 0.001f) bobHorizontal_ = 0.0f;
            
            // Плавный сброс фазы
            if (!isMoving) {
                bobPhase_ *= (1.0f - 5.0f * deltaTime);
            }
        }
    }

    /**
     * @brief Attempt to jump
     */
    void jump() {
        if (isGrounded_ && !isCrouching_) {
            verticalVelocity_ = PLAYER_JUMP_VELOCITY;
            isGrounded_ = false;
            isJumping_ = true;
            std::cout << "[Player] Jump!" << std::endl;
        }
    }

    /**
     * @brief Set sprint state
     */
    void setSprinting(bool sprinting) {
        // Can't sprint while crouching
        if (sprinting && isCrouching_) return;
        isSprinting_ = sprinting;
    }

    /**
     * @brief Set crouch state
     */
    void setCrouching(bool crouching) {
        if (crouching && !isCrouching_) {
            isCrouching_ = true;
            isSprinting_ = false;  // Stop sprinting when crouching
            targetEyeHeight_ = PLAYER_CROUCH_HEIGHT;
            std::cout << "[Player] Crouching" << std::endl;
        } else if (!crouching && isCrouching_) {
            // TODO: Check if there's enough headroom to stand up
            isCrouching_ = false;
            targetEyeHeight_ = PLAYER_STAND_HEIGHT;
            std::cout << "[Player] Standing" << std::endl;
        }
    }

    /**
     * @brief Toggle crouch state
     */
    void toggleCrouch() {
        setCrouching(!isCrouching_);
    }

    
    float getSpeedMultiplier() const {
        if (isCrouching_) return PLAYER_CROUCH_SPEED_MULT;
        if (isSprinting_) return PLAYER_SPRINT_MULTIPLIER;
        return 1.0f;
    }

   
    float getEyeHeight() const {
        return currentEyeHeight_ + verticalPos_ + bobVertical_;
    }

    float getHorizontalBob() const {
        return bobHorizontal_;
    }

    bool isMoving() const {
        return isMoving_;
    }

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

        // Horizontal rotation (yaw)
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
            viewAngle_ -= rotationSpeed * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
            viewAngle_ += rotationSpeed * deltaTime;
        }
        
        // Vertical look (pitch)
        float pitchSpeed = rotationSpeed * 0.5f;  // Медленнее чем горизонтальный
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
            pitchAngle_ = std::clamp(pitchAngle_ + pitchSpeed * deltaTime, PITCH_MIN, PITCH_MAX);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
            pitchAngle_ = std::clamp(pitchAngle_ - pitchSpeed * deltaTime, PITCH_MIN, PITCH_MAX);
        }

        // Normalize yaw angle
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
    float getPitchAngle() const { return pitchAngle_; }
    float getVerticalPos() const { return verticalPos_; }
    float getVerticalVelocity() const { return verticalVelocity_; }
    bool isGrounded() const { return isGrounded_; }
    bool isJumping() const { return isJumping_; }
    bool isSprinting() const { return isSprinting_; }
    bool isCrouching() const { return isCrouching_; }
    float getCurrentEyeHeight() const { return currentEyeHeight_; }
    
    // Setters
    void setPosition(sf::Vector2f pos) { 
        shape_.setPosition(pos); 
    }
    
    void setVerticalPos(float z) { verticalPos_ = z; }
    void setGrounded(bool grounded) { isGrounded_ = grounded; }
    
    void setViewAngle(float angle) { viewAngle_ = angle; }
    void setPitchAngle(float pitch) { 
        pitchAngle_ = std::clamp(pitch, PITCH_MIN, PITCH_MAX); 
    }
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
    float pitchAngle_;        // Vertical look angle (-1 to 1, 0 = forward)
    float mouseSensitivity_;  // Mouse sensitivity
    float lastMouseX_;        // Last mouse X position
    float lastMouseY_;        // Last mouse Y position
    bool firstMouse_;         // First mouse movement flag

    // Vertical movement (Z-axis)
    float verticalPos_;       // Height above floor (0 = on floor)
    float verticalVelocity_;  // Vertical speed
    bool isGrounded_;         // True if on ground
    bool isJumping_;          // True if jumping

    // Sprint
    bool isSprinting_;

    // Crouch
    bool isCrouching_;
    float currentEyeHeight_;  // Current eye height (smoothly transitions)
    float targetEyeHeight_;   // Target eye height (crouch or stand)

    // Head bobbing
    float bobPhase_;          // Current phase of bob cycle [0, 2PI]
    float bobVertical_;       // Current vertical bob offset
    float bobHorizontal_;     // Current horizontal bob offset  
    bool isMoving_;           // Is player currently moving
};