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
        , pitchAngle_(0.0f)
        , mouseSensitivity_(MOUSE_SENSITIVITY)
        , lastMouseX_(0.f)
        , lastMouseY_(0.f)
        , firstMouse_(true)
        , verticalPos_(0.0f)
        , verticalVelocity_(0.0f)
        , isGrounded_(true)
        , isJumping_(false)
        , isSprinting_(false)
        , isCrouching_(false)
        , currentEyeHeight_(PLAYER_STAND_HEIGHT)
        , targetEyeHeight_(PLAYER_STAND_HEIGHT)
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
        if (!isGrounded_) {
            verticalVelocity_ -= PLAYER_GRAVITY * deltaTime;
            
            if (verticalVelocity_ < -PLAYER_MAX_FALL_SPEED) {
                verticalVelocity_ = -PLAYER_MAX_FALL_SPEED;
            }
        }

        verticalPos_ += verticalVelocity_ * deltaTime;

        if (verticalPos_ <= floorHeight) {
            verticalPos_ = floorHeight;
            verticalVelocity_ = 0.0f;
            isGrounded_ = true;
            isJumping_ = false;
        } else {
            isGrounded_ = false;
        }

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
            float frequency = BOB_FREQUENCY_WALK;
            float amplitudeMultiplier = 1.0f;
            
            if (isSprinting_) {
                frequency = BOB_FREQUENCY_SPRINT;
                amplitudeMultiplier = BOB_SPRINT_MULTIPLIER;
            } else if (isCrouching_) {
                frequency = BOB_FREQUENCY_CROUCH;
                amplitudeMultiplier = BOB_CROUCH_MULTIPLIER;
            }
            
            bobPhase_ += frequency * deltaTime * 2.0f * M_PI;
            
            while (bobPhase_ >= 2.0f * M_PI) {
                bobPhase_ -= 2.0f * M_PI;
            }
            
            bobVertical_ = std::abs(std::sin(bobPhase_)) * BOB_AMPLITUDE_VERTICAL * amplitudeMultiplier;
            bobHorizontal_ = std::sin(bobPhase_ * 0.5f) * BOB_AMPLITUDE_HORIZONTAL * amplitudeMultiplier;
        } else {
            bobVertical_ *= (1.0f - 10.0f * deltaTime);
            bobHorizontal_ *= (1.0f - 10.0f * deltaTime);
            
            if (std::abs(bobVertical_) < 0.001f) bobVertical_ = 0.0f;
            if (std::abs(bobHorizontal_) < 0.001f) bobHorizontal_ = 0.0f;
            
            if (!isMoving) {
                bobPhase_ *= (1.0f - 5.0f * deltaTime);
            }
        }
    }

    void jump() {
        if (isGrounded_ && !isCrouching_) {
            verticalVelocity_ = PLAYER_JUMP_VELOCITY;
            isGrounded_ = false;
            isJumping_ = true;
            std::cout << "[Player] Jump!" << std::endl;
        }
    }

    void setSprinting(bool sprinting) {
        if (sprinting && isCrouching_) return;
        isSprinting_ = sprinting;
    }

    void setCrouching(bool crouching) {
        if (crouching && !isCrouching_) {
            isCrouching_ = true;
            isSprinting_ = false;
            targetEyeHeight_ = PLAYER_CROUCH_HEIGHT;
            std::cout << "[Player] Crouching" << std::endl;
        } else if (!crouching && isCrouching_) {
            isCrouching_ = false;
            targetEyeHeight_ = PLAYER_STAND_HEIGHT;
            std::cout << "[Player] Standing" << std::endl;
        }
    }

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
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) inputDir.y -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) inputDir.y += 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) inputDir.x -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) inputDir.x += 1.f;
        }

        if (inputDir.x != 0.f || inputDir.y != 0.f) {
            float length = std::sqrt(inputDir.x * inputDir.x + inputDir.y * inputDir.y);
            inputDir /= length; 
            velocity_ += inputDir * acceleration_ * deltaTime;
        }

        velocity_ -= velocity_ * friction_ * deltaTime;

        float currentSpeed = std::sqrt(velocity_.x * velocity_.x + velocity_.y * velocity_.y);
        if (currentSpeed > maxSpeed_) {
            velocity_ = (velocity_ / currentSpeed) * maxSpeed_;
        }

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

        viewAngle_ += deltaX * mouseSensitivity_;

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
        
        float pitchSpeed = rotationSpeed * 0.5f;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
            pitchAngle_ = std::clamp(pitchAngle_ + pitchSpeed * deltaTime, PITCH_MIN, PITCH_MAX);
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
            pitchAngle_ = std::clamp(pitchAngle_ - pitchSpeed * deltaTime, PITCH_MIN, PITCH_MAX);
        }

        while (viewAngle_ < 0) viewAngle_ += 2.0f * M_PI;
        while (viewAngle_ >= 2.0f * M_PI) viewAngle_ -= 2.0f * M_PI;
    }

    void draw(sf::RenderWindow& window) {
        window.draw(shape_);
    }

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
    float viewAngle_;
    float pitchAngle_;
    float mouseSensitivity_;
    float lastMouseX_;
    float lastMouseY_;
    bool firstMouse_;
    float verticalPos_;
    float verticalVelocity_;
    bool isGrounded_;
    bool isJumping_;
    bool isSprinting_;
    bool isCrouching_;
    float currentEyeHeight_;
    float targetEyeHeight_;
    float bobPhase_;
    float bobVertical_;
    float bobHorizontal_;
    bool isMoving_;
};