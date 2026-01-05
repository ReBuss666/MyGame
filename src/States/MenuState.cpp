#include "../../include/States/MenuState.h"
#include <iostream>
#include <cmath>

void MenuState::onEnter() {
    std::cout << "=== MenuState: Loading resources ===" << std::endl;

    // Debug: Print current working directory
    std::cout << "[MenuState] Loading logo from: ../assets/logo.png" << std::endl;
    
    logoTexture_ = ResourceManager::getInstance().getTexture("../assets/logo.png");
    
    if (!logoTexture_) {
        std::cerr << "[MenuState] FAILED to load logo texture!" << std::endl;
        std::cerr << "[MenuState] Make sure to run from 'build' folder!" << std::endl;
    } else {
        std::cout << "[MenuState] Logo loaded: " << logoTexture_->getSize().x << "x" << logoTexture_->getSize().y << std::endl;
        logoTexture_->setSmooth(false);
        
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

    fireEffect_ = std::make_unique<FireEffect>(window_WIDTH, window_HEIGHT, PixelSize);
    fireEffect_->triggerFlash();

    buttons_.reserve(3);
    buttons_.push_back(std::make_unique<Button>("Start Game", 
        sf::Vector2f(WINDOW_CENTER_X, MENU_BUTTON_START_Y)));
    buttons_.push_back(std::make_unique<Button>("Options", 
        sf::Vector2f(WINDOW_CENTER_X, MENU_BUTTON_START_Y + MENU_BUTTON_SPACING)));
    buttons_.push_back(std::make_unique<Button>("Exit", 
        sf::Vector2f(WINDOW_CENTER_X, MENU_BUTTON_START_Y + MENU_BUTTON_SPACING * 2)));

    buttonsFadeAlpha_ = 0.f;
    for (auto& button : buttons_) {
        button->setAlpha(0.f);
    }
    
    totalTime_ = 0.f;
    shinePos_ = -1.f;
    targetY_ = WINDOW_HEIGHT / 2.5f - LOGO_OFFSET_Y;
    logoAnimationComplete_ = false;
    
    std::cout << "=== MenuState: Loaded ===" << std::endl;
}

void MenuState::onExit() {
    std::cout << "=== MenuState: Releasing resources ===" << std::endl;
    buttons_.clear();
    fireEffect_.reset();
    shineSprite_.reset();
    logoSprite_.reset();
    logoTexture_ = nullptr;
    std::cout << "=== MenuState: Resources released ===" << std::endl;
}

void MenuState::pause() {
    std::cout << "=== MenuState: Paused ===" << std::endl;
    if (fireEffect_) {
        fireEffect_->disableFuel();
    }
}

void MenuState::resume() {
    std::cout << "=== MenuState: Resumed ===" << std::endl;
    if (fireEffect_) {
        fireEffect_->enableFuel();
    }
}

void MenuState::handleInput(const sf::Event& event) {
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

    if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
        mousePos_ = sf::Vector2f(
            static_cast<float>(mouseMoved->position.x),
            static_cast<float>(mouseMoved->position.y)
        );
    }
}

void MenuState::update(float deltaTime) {
    totalTime_ += deltaTime;

    if (fireEffect_) {
        fireEffect_->update();
    }

    if (logoSprite_) {
        sf::Vector2f currentPos = logoSprite_->getPosition();
        if (currentPos.y > targetY_) {
            logoSprite_->move({0.f, -LOGO_SPEED * deltaTime});
        } else {
            if (!logoAnimationComplete_) {
                logoAnimationComplete_ = true;
                std::cout << "=== Logo animation complete ===" << std::endl;
            }
            float offset = std::sin(totalTime_ * 2.0f) * 2.0f;
            logoSprite_->setPosition({WINDOW_CENTER_X, targetY_ + offset});
        }

        if (shineSprite_) {
            shineSprite_->setPosition(logoSprite_->getPosition());
        }
    }

    shinePos_ += deltaTime * 1.5f;
    if (shinePos_ > 2.f) shinePos_ = -1.f;
    
    float shineFactor = std::max(0.0f, 1.0f - std::abs(shinePos_ - 0.5f) * 4.0f);
    if (shineSprite_) {
        shineSprite_->setColor(sf::Color(255, 255, 255, 
            static_cast<std::uint8_t>(shineFactor * 150)));
    }

    if (logoAnimationComplete_ && buttonsFadeAlpha_ < 255.f) {
        buttonsFadeAlpha_ += 300.f * deltaTime;
        if (buttonsFadeAlpha_ > 255.f) {
            buttonsFadeAlpha_ = 255.f;
        }
        
        for (auto& button : buttons_) {
            button->setAlpha(buttonsFadeAlpha_);
        }
    }
    
    if (logoAnimationComplete_) {
        for (auto& button : buttons_) {
            button->update(mousePos_);
        }
    }
}

void MenuState::render(sf::RenderWindow& window) {
    // Logo on top of fire
    if (logoSprite_) {
        window.draw(*logoSprite_);
    }

    // Fire effect 
    if (fireEffect_) {
        fireEffect_->render(window);
    }

    if (shineSprite_) {
        window.draw(*shineSprite_, sf::BlendAdd);
    }

    // Buttons on top
    for (auto& button : buttons_) {
        button->render(window);
    }
}

void MenuState::skipAnimation() {
    if (logoAnimationComplete_ && buttonsFadeAlpha_ >= 255.f) {
        return;
    }

    std::cout << "=== Skipping animation ===" << std::endl;

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

void MenuState::handleButtonClick(const std::string& buttonName) {
    if (buttonName == "Start Game") {
        std::cout << ">>> Start Game clicked <<<" << std::endl;
        stateManager_->pushState("Play");
    } 
    else if (buttonName == "Options") {
        std::cout << ">>> Options clicked <<<" << std::endl;
        stateManager_->pushState("Options");
    } 
    else if (buttonName == "Exit") {
        std::cout << ">>> Exit clicked <<<" << std::endl;
        stateManager_->exitGame();
    }
}
