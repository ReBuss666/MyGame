#pragma once
#include "../Core/GameState.h"
#include "../World/SectorMap.h"
#include "../World/TestMapBuilder.h"
#include "../Core/StateManager.h"
#include "../Core/ResourceManager.h"
#include "../Utils/settings.h"
#include "../Entities/player.h"
#include "../Rendering/SectorRenderer.h"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <iostream>
#include <memory>
#include <cmath>

class PlayState : public GameState {
public:
    PlayState() = default; 

    void onEnter() override {
        std::cout << "=== PlayState (Sector Engine): Entering ===" << std::endl;
        
        // Setup render texture
        if (!renderTexture_.resize({INTERNAL_WIDTH, INTERNAL_HEIGHT})) {
            std::cerr << "ERROR: Failed to create render texture!" << std::endl;
        }
        renderTexture_.setSmooth(false); 
        
        renderSprite_ = std::make_unique<sf::Sprite>(renderTexture_.getTexture());

        // Create sector map
        std::cout << "[PlayState] Building sector map..." << std::endl;
        sectorMap_ = TestMapBuilder::buildSimpleStepMap();
        
        // Validate map
        if (!sectorMap_.validate()) {
            std::cerr << "ERROR: Sector map validation failed!" << std::endl;
        }

        // Create player in first sector (center of sector 0: 2 units in from corner)
        // Map uses 64 pixel units, so center of 4x4 sector at (0,0) is at (2*64, 2*64) = (128, 128)
        const float UNIT = 64.0f;
        player_ = std::make_unique<Player>(sf::Vector2f(2.0f * UNIT, 2.0f * UNIT));
        
        // Find initial sector
        updatePlayerSector();
        
        if (currentSector_) {
            std::cout << "[PlayState] Player starts in sector " << currentSector_->getId() << std::endl;
        } else {
            std::cerr << "WARNING: Player not in any sector!" << std::endl;
        }

        // Create sector renderer
        sectorRenderer_ = std::make_unique<SectorRenderer>(INTERNAL_WIDTH, INTERNAL_HEIGHT);
        sectorRenderer_->setRenderDistance(RAYCASTER_RENDER_DISTANCE);

        // Load textures
        wallTexture_ = ResourceManager::getInstance().getTexture(Assets::WALL_TEXTURE);
        if (wallTexture_) {
            wallTexture_->setRepeated(true); 
            wallTexture_->setSmooth(false);
            sectorRenderer_->setTexture(wallTexture_);
            std::cout << "[PlayState] Wall texture loaded: " << Assets::WALL_TEXTURE 
                      << " (" << wallTexture_->getSize().x << "x" << wallTexture_->getSize().y << ")" << std::endl;
        } else {
            std::cerr << "WARNING: Wall texture not found at: " << Assets::WALL_TEXTURE << std::endl;
            std::cerr << "         Walls will appear white. Check the file path." << std::endl;
        }

        mode3D_ = true;
        mouseLocked_ = false;

        std::cout << "=== PlayState: Ready ===" << std::endl;
        std::cout << "    Controls:" << std::endl;
        std::cout << "    - WASD: Move" << std::endl;
        std::cout << "    - Mouse/Arrow Keys: Look around" << std::endl;
        std::cout << "    - Space: Jump" << std::endl;
        std::cout << "    - Shift: Sprint" << std::endl;
        std::cout << "    - C/Ctrl: Crouch" << std::endl;
        std::cout << "    - TAB: Toggle 2D/3D view" << std::endl;
        std::cout << "    - M: Lock/unlock mouse" << std::endl;
        std::cout << "    - ESC: Pause menu" << std::endl;
    }
    
    void onExit() override {
        if (window_) {
            window_->setMouseCursorVisible(true);
            window_->setMouseCursorGrabbed(false);
        }

        sectorRenderer_.reset();
        player_.reset();
        renderSprite_.reset();
        wallTexture_ = nullptr;
        currentSector_ = nullptr;
    }

    void handleInput(const sf::Event& event) override {
        if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                if (mouseLocked_) {
                    unlockMouse();
                } else {
                    if (stateManager_) stateManager_->pushState("Pause");
                }
            }
            if (keyPressed->code == sf::Keyboard::Key::Tab) {
                mode3D_ = !mode3D_;
                std::cout << "[PlayState] Switched to " << (mode3D_ ? "3D" : "2D") << " mode" << std::endl;
                if (!mode3D_) {
                    unlockMouse();
                }
            }
            if (keyPressed->code == sf::Keyboard::Key::M && mode3D_) {
                mouseLocked_ = !mouseLocked_;
                if (mouseLocked_ && player_) {
                    lockMouse();
                } else {
                    unlockMouse();
                }
            }
            
            // Jump (Space)
            if (keyPressed->code == sf::Keyboard::Key::Space && player_) {
                player_->jump();
            }
            
            // Toggle crouch (C)
            if (keyPressed->code == sf::Keyboard::Key::C && player_) {
                player_->toggleCrouch();
            }
        }

        // Continuous key states for sprint and hold-to-crouch
        if (player_) {
            // Sprint (Shift)
            player_->setSprinting(sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
                                  sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift));
            
            // Hold crouch (Ctrl) - alternative to toggle
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
                player_->setCrouching(true);
            } else {
                player_->setCrouching(false);
            }
        }

        if (const auto* mouseMoved = event.getIf<sf::Event::MouseMoved>()) {
            if (mouseLocked_ && mode3D_ && window_) {
                sf::Vector2u windowSize = window_->getSize();
                sf::Vector2i center(windowSize.x / 2, windowSize.y / 2);

                float deltaX = static_cast<float>(mouseMoved->position.x - center.x);
                float deltaY = static_cast<float>(mouseMoved->position.y - center.y);

                // Горизонтальный поворот (yaw)
                if (std::abs(deltaX) > 0.5f) {
                    mouseRotation_ += deltaX * MOUSE_SENSITIVITY;
                }
                
                // Вертикальный поворот (pitch)
                if (std::abs(deltaY) > 0.5f) {
                    mousePitch_ += deltaY * MOUSE_SENSITIVITY_Y;  // Плюс: мышь вверх = смотреть вверх
                }

                if (std::abs(deltaX) > 5.f || std::abs(deltaY) > 5.f) {
                    sf::Mouse::setPosition(center, *window_);
                }
            }
        }

        if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
            if (mode3D_ && !mouseLocked_ && mousePressed->button == sf::Mouse::Button::Left) {
                lockMouse();
            }
        }
    }
    
    void update(float deltaTime) override {
        if (player_) {
            // Get current floor height for physics
            float currentFloorHeight = 0.0f;
            if (currentSector_) {
                currentFloorHeight = currentSector_->getFloorHeight();
            }
            
            // Update vertical movement (jumping/falling/crouching)
            player_->updateVertical(deltaTime, currentFloorHeight);
            
            // Update horizontal movement and check if moving
            bool wasMoving = updatePlayerMovement(deltaTime);
            
            // Update head bobbing (Doom-style camera sway)
            player_->updateHeadBob(deltaTime, wasMoving);
            
            player_->handleKeyboardRotation(deltaTime);

            // Apply mouse rotation (yaw)
            if (mouseLocked_ && std::abs(mouseRotation_) > 0.001f) {
                float currentAngle = player_->getViewAngle();
                player_->setViewAngle(currentAngle + mouseRotation_);
                mouseRotation_ = 0.f;
            }
            
            // Apply mouse pitch (vertical look)
            if (mouseLocked_ && std::abs(mousePitch_) > 0.001f) {
                float currentPitch = player_->getPitchAngle();
                player_->setPitchAngle(currentPitch + mousePitch_);
                mousePitch_ = 0.f;
            }

            // Update current sector
            updatePlayerSector();
            
            // Auto-step: if player moved to new sector with higher floor, snap up
            if (currentSector_) {
                float floorDiff = currentSector_->getFloorHeight() - player_->getVerticalPos();
                if (floorDiff > 0 && floorDiff <= MAX_STEP_HEIGHT && player_->isGrounded()) {
                    // Snap player up to new floor
                    player_->setVerticalPos(currentSector_->getFloorHeight());
                }
            }
        }
    }

    void render(sf::RenderWindow& window) override {
        if (!window_) {
            window_ = &window;
        }

        if (mode3D_) {
            // 3D Mode - render with SectorRenderer
            renderTexture_.clear(sf::Color::Black);
            
            if (sectorRenderer_ && player_ && currentSector_) {
                // Player eye height = base eye height + vertical position
                float eyeHeight = player_->getEyeHeight();
                float pitch = player_->getPitchAngle();
                sectorRenderer_->render(renderTexture_, 
                                       sectorMap_,
                                       player_->getPosition(), 
                                       player_->getViewAngle(),
                                       currentSector_,
                                       FOV_RADIANS,
                                       eyeHeight,
                                       pitch);
            } else if (!currentSector_) {
                // Player outside sectors - show warning
                std::cerr << "[PlayState] WARNING: Player not in any sector!" << std::endl;
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
            // 2D Mode - render sectors from top-down
            sectorMap_.render2D(window, player_->getPosition(), 1.0f);
            
            if (player_) {
                player_->draw(window);
                drawViewDirection(window);
            }

            // Highlight current sector
            if (currentSector_) {
                sf::FloatRect bounds = currentSector_->getBounds();
                sf::RectangleShape highlight;
                highlight.setSize({bounds.size.x, bounds.size.y});
                highlight.setPosition({bounds.position.x, bounds.position.y});
                highlight.setFillColor(sf::Color(255, 255, 0, 30)); // Yellow highlight
                highlight.setOutlineColor(sf::Color::Yellow);
                highlight.setOutlineThickness(2.0f);
                window.draw(highlight);
            }
        }
        
        drawHUD(window);
    }

private:
    std::unique_ptr<Player> player_;
    SectorMap sectorMap_;
    Sector* currentSector_ = nullptr;
    std::unique_ptr<SectorRenderer> sectorRenderer_;

    sf::RenderTexture renderTexture_;
    std::unique_ptr<sf::Sprite> renderSprite_;
    
    sf::Texture* wallTexture_ = nullptr;
    sf::RenderWindow* window_ = nullptr;

    bool mode3D_ = true;
    bool mouseLocked_ = false;
    float mouseRotation_ = 0.f;
    float mousePitch_ = 0.f;    // Вертикальный угол от мыши

    /**
     * @brief Update player's current sector
     */
    void updatePlayerSector() {
        Sector* newSector = sectorMap_.findSectorAt(player_->getPosition());
        
        if (newSector != currentSector_) {
            if (newSector) {
                std::cout << "[PlayState] Player entered sector " << newSector->getId() << std::endl;
            } else {
                std::cout << "[PlayState] WARNING: Player outside all sectors!" << std::endl;
            }
            currentSector_ = newSector;
        }
    }

    bool updatePlayerMovement(float deltaTime) {
        // Get input direction
        sf::Vector2f inputDir{0.f, 0.f};

        if (mode3D_) {
            // 3D Mode: WASD relative to view direction
            float viewAngle = player_->getViewAngle();
            
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) {
                inputDir.x += std::cos(viewAngle);
                inputDir.y += std::sin(viewAngle);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) {
                inputDir.x -= std::cos(viewAngle);
                inputDir.y -= std::sin(viewAngle);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) {
                inputDir.x += std::cos(viewAngle - M_PI / 2.f);
                inputDir.y += std::sin(viewAngle - M_PI / 2.f);
            }
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) {
                inputDir.x += std::cos(viewAngle + M_PI / 2.f);
                inputDir.y += std::sin(viewAngle + M_PI / 2.f);
            }
        } else {
            // 2D Mode: Axis-aligned movement
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::W)) inputDir.y -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::S)) inputDir.y += 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::A)) inputDir.x -= 1.f;
            if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::D)) inputDir.x += 1.f;
        }

        // Check if any movement input
        bool hasInput = (inputDir.x != 0.f || inputDir.y != 0.f);
        
        // Normalize
        if (hasInput) {
            float length = std::sqrt(inputDir.x * inputDir.x + inputDir.y * inputDir.y);
            inputDir /= length;
        }

        // Calculate speed with sprint/crouch modifier
        float speedMultiplier = player_->getSpeedMultiplier();
        float currentSpeed = PLAYER_SPEED * speedMultiplier;
        
        // Calculate new position
        sf::Vector2f currentPos = player_->getPosition();
        sf::Vector2f newPos = currentPos + inputDir * currentSpeed * deltaTime;

        bool actuallyMoved = false;

        // Check collision with sector walls (allow climbing steps while jumping/falling)
        bool isInAir = !player_->isGrounded();
        bool blocked = sectorMap_.isBlocked(currentPos, newPos, PLAYER_WIDTH / 2.0f, isInAir);

        if (!blocked && hasInput) {
            // Move player to new position
            player_->setPosition(newPos);
            actuallyMoved = true;
        } else if (hasInput) {
            // Try sliding along walls (try X and Y separately)
            sf::Vector2f slideX = sf::Vector2f(newPos.x, currentPos.y);
            sf::Vector2f slideY = sf::Vector2f(currentPos.x, newPos.y);
            
            if (!sectorMap_.isBlocked(currentPos, slideX, PLAYER_WIDTH / 2.0f, isInAir)) {
                player_->setPosition(slideX);
                actuallyMoved = true;
            } else if (!sectorMap_.isBlocked(currentPos, slideY, PLAYER_WIDTH / 2.0f, isInAir)) {
                player_->setPosition(slideY);
                actuallyMoved = true;
            }
            // If both blocked, don't move
        }
        
        return actuallyMoved;
    }

    void lockMouse() {
        if (!window_) return;

        mouseLocked_ = true;
        mouseRotation_ = 0.f;
        
        window_->setMouseCursorVisible(false);
        window_->setMouseCursorGrabbed(true);

        sf::Vector2u windowSize = window_->getSize();
        sf::Mouse::setPosition(
            sf::Vector2i(windowSize.x / 2, windowSize.y / 2),
            *window_
        );

        if (player_) {
            player_->resetMouseTracking();
        }

        std::cout << "[PlayState] Mouse locked" << std::endl;
    }
    
    void unlockMouse() {
        if (!window_) return;

        mouseLocked_ = false;
        window_->setMouseCursorVisible(true);
        window_->setMouseCursorGrabbed(false);

        std::cout << "[PlayState] Mouse unlocked" << std::endl;
    }

    void drawCrosshair(sf::RenderWindow& window) {
       float centerX = window.getSize().x / 2.f;
       float centerY = window.getSize().y / 2.f;
       sf::RectangleShape hLine({20.f, 2.f}); 
       hLine.setPosition({centerX - 10.f, centerY - 1.f});
       hLine.setFillColor(sf::Color::White);
       
       sf::RectangleShape vLine({2.f, 20.f}); 
       vLine.setPosition({centerX - 1.f, centerY - 10.f});
       vLine.setFillColor(sf::Color::White);
       
       window.draw(hLine); 
       window.draw(vLine);
    }

    void drawViewDirection(sf::RenderWindow& window) {
        if (!player_) return;
        sf::Vector2f pos = player_->getPosition();
        float angle = player_->getViewAngle();
        sf::Vector2f endPos(pos.x + std::cos(angle) * 50.f, pos.y + std::sin(angle) * 50.f);
        sf::Vertex line[] = { 
            sf::Vertex(pos, sf::Color::Yellow), 
            sf::Vertex(endPos, sf::Color::Yellow) 
        };
        window.draw(line, 2, sf::PrimitiveType::Lines);
    }

    void drawHUD(sf::RenderWindow& window) {
        static sf::Font* font = ResourceManager::getInstance().getFont(Assets::FONT_PRIMARY);
        if (font) {
            sf::Text text(*font);
            
            // Mode and mouse status
            std::string modeText = mode3D_ ? "3D Mode (Sector Engine)" : "2D Mode";
            std::string mouseText = mouseLocked_ ? " | Mouse: LOCKED [ESC]" : " | Click/[M] to lock";
            
            // Player state info
            std::string stateText = "";
            if (player_) {
                if (player_->isSprinting()) stateText += " [SPRINT]";
                if (player_->isCrouching()) stateText += " [CROUCH]";
                if (player_->isJumping()) stateText += " [JUMP]";
                if (!player_->isGrounded()) stateText += " [AIR]";
            }
            
            // Current sector info
            std::string sectorText = "";
            if (currentSector_) {
                sectorText = "\nSector " + std::to_string(currentSector_->getId()) + 
                            " | Floor: " + std::to_string(currentSector_->getFloorHeight()) +
                            "m | Ceiling: " + std::to_string(currentSector_->getCeilingHeight()) + "m";
                if (player_) {
                    sectorText += " | Eye: " + std::to_string(player_->getEyeHeight()).substr(0, 4) + "m";
                }
            } else {
                sectorText = "\nWARNING: Outside all sectors!";
            }
            
            // Controls hint
            std::string controlsText = "\n[Space] Jump | [Shift] Sprint | [C/Ctrl] Crouch";
            
            text.setString(modeText + mouseText + stateText + sectorText + controlsText);
            text.setCharacterSize(20);
            text.setPosition({10.f, 10.f});
            text.setFillColor(sf::Color::White);
            
            // Add background for readability
            sf::FloatRect textBounds = text.getGlobalBounds();
            sf::RectangleShape background;
            background.setSize({textBounds.size.x + 20.f, textBounds.size.y + 20.f});
            background.setPosition({textBounds.position.x - 10.f, textBounds.position.y - 10.f});
            background.setFillColor(sf::Color(0, 0, 0, 150));
            
            window.draw(background);
            window.draw(text);
        }
    }
};