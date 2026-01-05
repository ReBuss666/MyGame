#pragma once 
#include "../Core/GameState.h"
#include "../Core/StateManager.h"
#include "../Core/ResourceManager.h"
#include "../Rendering/FireEffect.h"
#include "../UI/Button.h"
#include "../Utils/settings.h"
#include <vector>
#include <memory>
#include <iostream>
#include <cmath>

class MenuState : public GameState {
public:
    void onEnter() override {
        std::cout << "=== MenuState: Loading resources ===" << std::endl;

        // Load texture from ResourceManager (cached)
        logoTexture_ = ResourceManager::getInstance().getTexture("../assets/logo.png");
        
        if (!logoTexture_) {
            std::cerr << "[MenuState] Failed to load logo texture" << std::endl;
        } else {
            logoTexture_->setSmooth(false);
            
            // Create sprite with cached texture
            logoSprite_ = std::make_unique<sf::Sprite>(*logoTexture_);

            sf::FloatRect bounds = logoSprite_->getLocalBounds();
            logoSprite_->setOrigin({bounds.size.x / 2.f, bounds.size.y / 2.f});
            logoSprite_->setPosition({WINDOW_CENTER_X, WINDOW_HEIGHT + bounds.size.y / 2.f + 10.f});
            logoSprite_->setScale({2.0f, 2.0f});

            shineSprite_ = std::make_unique<sf::Sprite>(*logoTexture_);
            shineSprite_->setOrigin(logoSprite_->getOrigin());
            shineSprite_->setScale(logoSprite_->getScale());
            shineSprite_->setColor(sf::Color(255, 255, 255, 170));
        }

        // Use correct constant names
        fireEffect_ = std::make_unique<FireEffect>(window_WIDTH, window_HEIGHT, PixelSize);
        fireEffect_->triggerFlash();

        // Create buttons
        buttons_.reserve(3);
        buttons_.push_back(std::make_unique<Button>("Start Game", 
            sf::Vector2f(WINDOW_CENTER_X, MENU_BUTTON_START_Y)));
        buttons_.push_back(std::make_unique<Button>("Options", 
            sf::Vector2f(WINDOW_CENTER_X, MENU_BUTTON_START_Y + MENU_BUTTON_SPACING)));
        buttons_.push_back(std::make_unique<Button>("Exit", 
            sf::Vector2f(WINDOW_CENTER_X, MENU_BUTTON_START_Y + MENU_BUTTON_SPACING * 2)));

        // Set initial button alpha
        buttonsFadeAlpha_ = 0.f;
        for (auto& button : buttons_) {
            button->setAlpha(0.f);
        }
        
        totalTime_ = 0.f;
        shinePos_ = -1.f;
        targetY_ = WINDOW_HEIGHT / 2.5f - LOGO_OFFSET_Y;
        logoAnimationComplete_ = false;
        
        std::cout << "=== MenuState: Loaded (using cached resources) ===" << std::endl;
    }

    void onExit() override {
        std::cout << "=== MenuState: Releasing resources ===" << std::endl;
        
        buttons_.clear();
        fireEffect_.reset();
        shineSprite_.reset();
        logoSprite_.reset();
        logoTexture_ = nullptr; // Just a pointer, actual texture stays in ResourceManager

        std::cout << "=== MenuState: Resources released (textures cached) ===" << std::endl;
    }

    void pause() override {
        std::cout << "=== MenuState: Paused (Play on top) ===" << std::endl;
        if (fireEffect_) {
            fireEffect_->disableFuel();
        }
    }

    void resume() override {
        std::cout << "=== MenuState: Resumed ===" << std::endl;
        if (fireEffect_) {
            fireEffect_->enableFuel();
        }
    }

    void handleInput(const sf::Event& event) override {
        // Skip animation on any input
        if (!logoAnimationComplete_ || buttonsFadeAlpha_ < 255.f) {
            if (event.is<sf::Event::KeyPressed>() || event.is<sf::Event::MouseButtonPressed>()) {
                skipAnimation();
                return;
            }

            if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
                mousePos_ = sf::Vector2f(
                    static_cast<float>(mouseMoved->position.x),
                    static_cast<float>(mouseMoved->position.y)
                );
            }
            return;
        }

        // Handle button clicks
        if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
            sf::Vector2f mousePos(
                static_cast<float>(mousePressed->position.x),
                static_cast<float>(mousePressed->position.y)
            );

            for (auto& button : buttons_) {
                if (button && button->isClicked(mousePos)) {
                    handleButtonClick(button->getName());
                    return;
                }
            }
        }

        // Track mouse position
        if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
            mousePos_ = sf::Vector2f(
                static_cast<float>(mouseMoved->position.x),
                static_cast<float>(mouseMoved->position.y)
            );
        }
    }

    void update(float deltaTime) override {
        totalTime_ += deltaTime;

        // Update fire effect
        if (fireEffect_) {
            fireEffect_->update();
        }

        // Logo animation
        if (logoSprite_) {
            sf::Vector2f currentPos = logoSprite_->getPosition();
            if (currentPos.y > targetY_) {
                logoSprite_->move({0.f, -LOGO_SPEED * deltaTime});
            } else {
                if (!logoAnimationComplete_) {
                    logoAnimationComplete_ = true;
                    std::cout << "=== Logo animation complete - buttons fading in ===" << std::endl;
                }
                // Subtle floating effect
                float offset = std::sin(totalTime_ * 2.0f) * 2.0f;
                logoSprite_->setPosition({WINDOW_CENTER_X, targetY_ + offset});
            }

            if (shineSprite_) {
                shineSprite_->setPosition(logoSprite_->getPosition());
            }
        }

        // Shine effect
        shinePos_ += deltaTime * 1.5f;
        if (shinePos_ > 2.f) shinePos_ = -1.f;
        
        float shineFactor = std::max(0.0f, 1.0f - std::abs(shinePos_ - 0.5f) * 4.0f);
        if (shineSprite_) {
            shineSprite_->setColor(sf::Color(255, 255, 255, 
                static_cast<std::uint8_t>(shineFactor * 150)));
        }

        // Buttons fade-in
        if (logoAnimationComplete_ && buttonsFadeAlpha_ < 255.f) {
            buttonsFadeAlpha_ += 300.f * deltaTime;
            if (buttonsFadeAlpha_ > 255.f) {
                buttonsFadeAlpha_ = 255.f;
            }
            
            for (auto& button : buttons_) {
                button->setAlpha(buttonsFadeAlpha_);
            }
        }
        
        // Update buttons
        if (logoAnimationComplete_) {
            for (auto& button : buttons_) {
                button->update(mousePos_);
            }
        }
    }
    
    void render(sf::RenderWindow& window) override {
        if (logoSprite_) {
            window.draw(*logoSprite_);
        }

        if (shineSprite_) {
            window.draw(*shineSprite_, sf::BlendAdd);
        }

        if (fireEffect_) {
            fireEffect_->render(window);
        }

        for (auto& button : buttons_) {
            button->render(window);
        }
    }

private:
    sf::Texture* logoTexture_; // Pointer to cached texture in ResourceManager
    std::unique_ptr<sf::Sprite> logoSprite_;
    std::unique_ptr<sf::Sprite> shineSprite_;
    std::unique_ptr<FireEffect> fireEffect_;
    std::vector<std::unique_ptr<Button>> buttons_;

    sf::Vector2f mousePos_{0.f, 0.f};
    float totalTime_ = 0.f;
    float shinePos_ = -1.0f;
    float targetY_ = 0.f;
    
    bool logoAnimationComplete_ = false;
    float buttonsFadeAlpha_ = 0.f;

    void skipAnimation() {
        if (logoAnimationComplete_ && buttonsFadeAlpha_ >= 255.f) {
            return; // Already complete
        }

        std::cout << "=== Skipping logo animation ===" << std::endl;

        if (logoSprite_) {
            logoSprite_->setPosition({WINDOW_CENTER_X, targetY_});
        }

        if (fireEffect_) {
            fireEffect_->triggerFlash();
        }

        logoAnimationComplete_ = true;
        buttonsFadeAlpha_ = 255.f;
        
        for (auto& button : buttons_) {
            button->setAlpha(255.f);
        }
    }

    void handleButtonClick(const std::string& buttonName) {
        if (buttonName == "Start Game") {
            std::cout << ">>> Start Game button clicked! <<<" << std::endl;
            stateManager_->pushState("Play");
        } 
        else if (buttonName == "Options") {
            std::cout << ">>> Options button clicked! <<<" << std::endl;
            stateManager_->pushState("Options");
        } 
        else if (buttonName == "Exit") {
            std::cout << ">>> Exit button clicked! <<<" << std::endl;
            stateManager_->exitGame();
        }
    }
};