#pragma once
#include "../Core/GameState.h"
#include "../World/Map.h"
#include "../Core/StateManager.h"
#include "../Core/ResourceManager.h"
#include "../Utils/settings.h"
#include "../Entities/player.h"
#include "../Rendering/RaycasterRenderer.h"
#include <iostream>
#include <memory>

class PlayState : public GameState {
public:
    void onEnter() override {
        std::cout << "=== PlayState: Entering Play State ===" << std::endl;
        
        // Инициализируем карту
        map_ = Map();
        std::cout << "=== PlayState: Map initialized ===" << std::endl;
        
        // Инициализируем игрока
        player_ = std::make_unique<Player>(sf::Vector2f(100.f, 100.f));
        std::cout << "=== PlayState: Player initialized ===" << std::endl;

        // Инициализируем рейкастерный рендерер
        raycaster_ = std::make_unique<RayCasterRenderer>(WINDOW_WIDTH, WINDOW_HEIGHT);
        raycaster_->setRenderDistance(RAYCASTER_RENDER_DISTANCE);
        raycaster_->setWallHeight(RAYCASTER_WALL_HEIGHT);
        std::cout << "=== PlayState: RayCasterRenderer initialized ===" << std::endl;

        // start with 3D mode
        mode3D_ = true;
        mouseLocked_ = false;

        std::cout << "=== PlayState: Ready (3D mode, mouse unlocked) ===" << std::endl;
    }
    
    void onExit() override {
        std::cout << "=== PlayState: Exiting Play State ===" << std::endl;
        raycaster_.reset();
        player_.reset();
    }

    void handleInput(const sf::Event& event) override {
        // Handle pause menu toggle
        if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                std::cout << ">>> Opening pause menu <<<" << std::endl;
                stateManager_->pushState("Pause");
            }

            // Toggle 2D/3D mode Tab
            if (keyPressed->code == sf::Keyboard::Key::Tab) {
                mode3D_ = !mode3D_;
                std::cout << ">>> Toggled mode: " << (mode3D_ ? "3D" : "2D") << " <<<" << std::endl;
            }

            // Toggle mouse lock M
            if (keyPressed->code == sf::Keyboard::Key::M && mode3D_) {
                mouseLocked_ = !mouseLocked_;
                if (mouseLocked_) {
                   player_->resetMouseTracking();
                   std::cout << ">>> Mouse locked"  << std::endl;
                } else {
                    std::cout << " Mouse unlocked " << std::endl;
                }
            }
        }

        // Обработка движения мыши (только если заблокирована)
        if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
            if (mouseLocked_ && mode3D_) {
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
            // === 3D РЕЖИМ ===
            if (raycaster_ && player_) {
                raycaster_->render(window, map_, 
                                   player_->getPosition(), 
                                   player_->getViewAngle(),
                                   FOV_RADIANS);
            }

            // Отрисовка прицела (crosshair)
            drawCrosshair(window);

        } else {
            // === 2D РЕЖИМ ===
            map_.render(window);
            
            if (player_) {
                player_->draw(window);
                
                // Отрисовка направления взгляда
                drawViewDirection(window);
            }
        }

        // HUD
        drawHUD(window);
    }

private:
    std::unique_ptr<Player> player_;
    Map map_;  // Map хранится как значение, не указатель
    std::unique_ptr<RayCasterRenderer> raycaster_;

    bool mode3D_; //true = 3D, false = 2D
    bool mouseLocked_; // Mouse look enabled
    
    // Draw simple crosshair in the center of the screen
    void drawCrosshair(sf::RenderWindow& window) {
       float centerX = WINDOW_WIDTH / 2.f;
       float centerY = WINDOW_HEIGHT / 2.f;
       float size = 10.f;
       float thickness = 2.f;

       // Horizontal line
       sf::RectangleShape hLine({size * 2, thickness});
       hLine.setPosition({centerX - size, centerY - thickness / 2.f});
       hLine.setFillColor(sf::Color(255, 255, 255, 180));
       window.draw(hLine);

       // Vertical line
       sf::RectangleShape vLine({thickness, size * 2});
       vLine.setPosition({centerX - thickness / 2.f, centerY - size});
       vLine.setFillColor(sf::Color(255, 255, 255, 180));
       window.draw(vLine);
    }

    // Draw player's view direction in 2D mode

    void drawViewDirection(sf::RenderWindow& window) {
        sf::Vector2f pos = player_->getPosition();
        float angle = player_->getViewAngle();
        float length = 50.0f;

        sf::Vector2f endPos(
            pos.x + std::cos(angle) * length,
            pos.y + std::sin(angle) * length
        );

        // Линия взгляда
        sf::Vertex line[] = {
            sf::Vertex(pos, sf::Color::Yellow),
            sf::Vertex(endPos, sf::Color::Yellow)
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);

        // Точка в конце
        sf::CircleShape dot(3.0f);
        dot.setOrigin({3.0f, 3.0f});
        dot.setPosition(endPos);
        dot.setFillColor(sf::Color::Yellow);
        window.draw(dot);
    }

    // Draw HUD elements
    void drawHUD(sf::RenderWindow& window) {
        static sf::Font* font = nullptr;
        if (!font) {
            font = ResourceManager::getInstance().getFont(Assets::FONT_PRIMARY);
            if (!font) {
                font = ResourceManager::getInstance().getFont(Assets::FONT_FALLBACK_1);
            }
        }

        if (font) {
            // Режим отображения
            sf::Text modeText(*font);
            modeText.setString(mode3D_ ? "3D Mode" : "2D Mode");
            modeText.setCharacterSize(20);
            modeText.setFillColor(sf::Color::White);
            modeText.setPosition({10.f, 10.f});
            window.draw(modeText);

            // Подсказка управления
            sf::Text hintText(*font);
            if (mode3D_) {
                hintText.setString(mouseLocked_ ? 
                    "Tab: 2D | M: Unlock Mouse | Arrows: Rotate" : 
                    "Tab: 2D | M: Lock Mouse | Arrows: Rotate");
            } else {
                hintText.setString("Tab: 3D | Arrows: Change View");
            }
            hintText.setCharacterSize(16);
            hintText.setFillColor(sf::Color(255, 255, 255, 180));
            hintText.setPosition({10.f, 40.f});
            window.draw(hintText);
        }
    }
};