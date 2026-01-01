#pragma once
#include "../Core/GameState.h"
#include "../World/Map.h"
#include "../Core/StateManager.h"
#include "../Core/ResourceManager.h"
#include "../Utils/settings.h"
#include "../Entities/player.h"
#include "../Rendering/RaycasterRenderer.h"
#include <SFML/Graphics.hpp>
#include <iostream>
#include <memory>
#include <cmath>

class PlayState : public GameState {
public:
    PlayState() = default; 

    void onEnter() override {
        std::cout << "=== PlayState: Entering ===" << std::endl;
        
        // Используем внутренние константы
        if (!renderTexture_.resize({INTERNAL_WIDTH, INTERNAL_HEIGHT})) {
            std::cerr << "ERROR: Failed to create render texture!" << std::endl;
        }
        renderTexture_.setSmooth(false); 
        
        renderSprite_ = std::make_unique<sf::Sprite>(renderTexture_.getTexture());

        map_ = Map();
        player_ = std::make_unique<Player>(sf::Vector2f(100.f, 100.f));
        
        raycaster_ = std::make_unique<RayCasterRenderer>(INTERNAL_WIDTH, INTERNAL_HEIGHT);
        raycaster_->setRenderDistance(RAYCASTER_RENDER_DISTANCE);
        raycaster_->setWallHeight(RAYCASTER_WALL_HEIGHT);

        // Текстуры стен
        wallTexture_ = ResourceManager::getInstance().getTexture(Assets::WALL_TEXTURE);
        if (wallTexture_) {
            wallTexture_->setRepeated(true); 
            wallTexture_->setSmooth(false);
            raycaster_->setTexture(wallTexture_);
        } else {
            std::cerr << "WARNING: Wall texture not found, using solid colors." << std::endl;
        }

        mode3D_ = true;
        mouseLocked_ = false;

        std::cout << "=== PlayState: Ready ===" << std::endl;
    }
    
    void onExit() override {
        raycaster_.reset();
        player_.reset();
        renderSprite_.reset();
        wallTexture_ = nullptr; 
    }

    void handleInput(const sf::Event& event) override {
        if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                if (stateManager_) stateManager_->pushState("Pause");
            }
            if (keyPressed->code == sf::Keyboard::Key::Tab) {
                mode3D_ = !mode3D_;
            }
            if (keyPressed->code == sf::Keyboard::Key::M && mode3D_) {
                mouseLocked_ = !mouseLocked_;
                if (mouseLocked_ && player_) player_->resetMouseTracking();
            }
        }

        if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
            if (mouseLocked_ && mode3D_ && player_) {
                player_->handleMouseLook(static_cast<float>(mouseMoved->position.x));
            }
        }
    }
    
    void update(float deltaTime) override {
        if (player_) {
            player_->update(deltaTime, map_, mode3D_);
            player_->handleKeyboardRotation(deltaTime);
        }
    }

    void render(sf::RenderWindow& window) override {
        if (mode3D_) {
            renderTexture_.clear(sf::Color::Black);
            
            if (raycaster_ && player_) {
                raycaster_->render(renderTexture_, map_, 
                                   player_->getPosition(), 
                                   player_->getViewAngle(),
                                   FOV_RADIANS);
            }
            renderTexture_.display();

            if (renderSprite_) {
                float scaleX = static_cast<float>(window.getSize().x) / static_cast<float>(INTERNAL_WIDTH);
                float scaleY = static_cast<float>(window.getSize().y) / static_cast<float>(INTERNAL_HEIGHT);
                renderSprite_->setScale({scaleX, scaleY});
                window.draw(*renderSprite_);
            }
            drawCrosshair(window);
        } else {
            map_.render(window);
            if (player_) {
                player_->draw(window);
                drawViewDirection(window);
            }
        }
        drawHUD(window);
    }

private:
    static constexpr unsigned int INTERNAL_WIDTH = 480;
    static constexpr unsigned int INTERNAL_HEIGHT = 300;

    std::unique_ptr<Player> player_;
    Map map_;
    std::unique_ptr<RayCasterRenderer> raycaster_;

    sf::RenderTexture renderTexture_;
    std::unique_ptr<sf::Sprite> renderSprite_;
    
    sf::Texture* wallTexture_ = nullptr;

    bool mode3D_ = true;
    bool mouseLocked_ = false;
    
    void drawCrosshair(sf::RenderWindow& window) {
       float centerX = window.getSize().x / 2.f;
       float centerY = window.getSize().y / 2.f;
       sf::RectangleShape hLine({20.f, 2.f}); hLine.setPosition({centerX - 10.f, centerY - 1.f});
       sf::RectangleShape vLine({2.f, 20.f}); vLine.setPosition({centerX - 1.f, centerY - 10.f});
       window.draw(hLine); window.draw(vLine);
    }

    void drawViewDirection(sf::RenderWindow& window) {
        if (!player_) return;
        sf::Vector2f pos = player_->getPosition();
        float angle = player_->getViewAngle();
        sf::Vector2f endPos(pos.x + std::cos(angle) * 50.f, pos.y + std::sin(angle) * 50.f);
        sf::Vertex line[] = { sf::Vertex(pos, sf::Color::Yellow), sf::Vertex(endPos, sf::Color::Yellow) };
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }

    void drawHUD(sf::RenderWindow& window) {
        static sf::Font* font = ResourceManager::getInstance().getFont(Assets::FONT_PRIMARY);
        if (font) {
            sf::Text text(*font);
            text.setString(mode3D_ ? "3D Mode" : "2D Mode");
            text.setCharacterSize(20);
            text.setPosition({10.f, 10.f});
            window.draw(text);
        }
    }
};