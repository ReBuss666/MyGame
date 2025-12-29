#pragma once
#include "../Core/GameState.h"
#include "../World/map.h"
#include "../Core/StateManager.h"
#include "../Utils/settings.h"
#include "../Entities/player.h"
#include <iostream>

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
    }
    
    void onExit() override {
        std::cout << "=== PlayState: Exiting Play State ===" << std::endl;
        player_.reset();
    }

    void handleInput(const sf::Event& event) override {
        // Handle player input
        if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                std::cout << ">>> Opening pause menu <<<" << std::endl;
                stateManager_->pushState("Pause");
            }
        }
    }
    
    void update(float deltaTime) override {
        if (player_) {
            player_->update(deltaTime, map_);
        }
    }

    void render(sf::RenderWindow& window) override {
        // Отладочный вывод для диагностики
        static int renderCount = 0;
        if (renderCount++ < 5) {
            std::cout << "PlayState::render() called - frame " << renderCount << std::endl;
        }
        
        // Рендерим карту
        map_.render(window);
        
        // Рендерим игрока
        if (player_) {
            player_->draw(window);
        }
    }

private:
    std::unique_ptr<Player> player_;
    Map map_;  // Map хранится как значение, не указатель
};