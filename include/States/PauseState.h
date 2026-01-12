#pragma once 
#include "../Core/GameState.h"
#include "../Core/StateManager.h"
#include "../UI/Button.h"
#include "../Utils/settings.h"
#include <vector>
#include <memory>
#include <iostream>

class PauseState : public GameState {
public: 
    void onEnter() override {
        std::cout << "=== PauseState: Game Paused ===" << std::endl;

        buttons_.reserve(3);
        buttons_.push_back(std::make_unique<Button>("Resume", sf::Vector2f(WINDOW_CENTER_X, 500.f)));
        buttons_.push_back(std::make_unique<Button>("Main Menu", sf::Vector2f(WINDOW_CENTER_X, 600.f)));
        buttons_.push_back(std::make_unique<Button>("Exit", sf::Vector2f(WINDOW_CENTER_X, 700.f)));
    }

    void onExit() override {
        std::cout << "=== PauseState: Exiting Pause Menu ===" << std::endl;
        buttons_.clear();
    }

    void handleInput(const sf::Event& event) override {
        if (exiting_) return;
        
        if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
            sf::Vector2f mousePos(
                static_cast<float>(mousePressed->position.x),
                static_cast<float>(mousePressed->position.y)
            );
            
            for (auto& button : buttons_) {
                if (button->isClicked(mousePos)) {
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
        
        if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
            if (keyPressed->code == sf::Keyboard::Key::Escape) {
                exiting_ = true;
                stateManager_->popState();
                return;
            }
        }
    }
    
    void update(float deltaTime) override {
        for (auto& button : buttons_) {
            button->update(mousePos_, deltaTime);
        }
    }
    
    void render(sf::RenderWindow& window) override {
        sf::RectangleShape overlay({static_cast<float>(WINDOW_WIDTH), static_cast<float>(WINDOW_HEIGHT)});
        overlay.setFillColor(sf::Color(0, 0, 0, 180));
        window.draw(overlay);
        
        for (auto& button : buttons_) {
            button->render(window);
        }
    }

private:
    std::vector<std::unique_ptr<Button>> buttons_;
    sf::Vector2f mousePos_{0.f, 0.f};
    bool exiting_ = false;

    void handleButtonClick(const std::string& buttonName) {
        if (buttonName == "Resume") {
            std::cout << ">>> Resuming Game <<<" << std::endl;
            exiting_ = true;
            stateManager_->popState();
        } 
        else if (buttonName == "Main Menu") {
            std::cout << ">>> Returning to Main Menu <<<" << std::endl;
            exiting_ = true;
            stateManager_->clearAndSwitchTo("Menu");
        } 
        else if (buttonName == "Exit") {
            std::cout << ">>> Exiting Game <<<" << std::endl;
            exiting_ = true;
            stateManager_->exitGame();
        }
    }
};