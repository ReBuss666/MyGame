#pragma once 
#include "GameState.h"
#include <stack>
#include <memory>
#include <unordered_map>
#include <string>
#include <functional>
#include <iostream>

class StateManager {
public: 
    StateManager() : window_(nullptr) {}

    void setWindow(sf::RenderWindow* window) {
        window_ = window;
    }

    template<typename T>
    void registerState(const std::string& name) {
        stateFactories_[name] = []() -> std::unique_ptr<GameState> {
            return std::make_unique<T>();
        };
        std::cout << "[StateManager] Registered state: " << name << std::endl;
    }

    // Register state with string parameter (e.g., map path)
    template<typename T>
    void registerStateWithParam(const std::string& name, const std::string& defaultParam) {
        stateFactories_[name] = [this, defaultParam]() -> std::unique_ptr<GameState> {
            // Use currentMapPath_ if set, otherwise use defaultParam
            const std::string& pathToUse = currentMapPath_.empty() ? defaultParam : currentMapPath_;
            return std::make_unique<T>(pathToUse);
        };
        std::cout << "[StateManager] Registered state with param: " << name << std::endl;
    }

    // Set/update map path for Play state
    void setMapPath(const std::string& path) {
        currentMapPath_ = path;
    }

    const std::string& getMapPath() const {
        return currentMapPath_;
    }

    // Generic switch with parameter
    template<typename T>
    void switchToWithParam(const std::string& stateName, const std::string& param) {
        std::cout << "[StateManager] Switching to: " << stateName << " with param: " << param << std::endl;
        
        while (!states_.empty()) {
            states_.top()->onExit();
            states_.pop();
        }

        auto newState = std::make_unique<T>(param);
        if (newState) {
            newState->stateManager_ = this;
            newState->onEnter();
            states_.push(std::move(newState));
            std::cout << "[StateManager] Switch complete" << std::endl;
        }
    }

    void exitGame() {
        std::cout << "[StateManager] Closing game..." << std::endl;
        if (window_) {
            window_->close();
        }
    }

    void switchTo(const std::string& stateName) {
        std::cout << "[StateManager] Switching to: " << stateName << std::endl;
        
        // Clean up old states
        while (!states_.empty()) {
            states_.top()->onExit();
            states_.pop();
        }
        

        auto newState = createState(stateName);
        if (newState) {
            newState->stateManager_ = this;
            newState->onEnter();
            states_.push(std::move(newState));
            std::cout << "[StateManager] Switch complete" << std::endl;
        } else {
            std::cerr << "[StateManager] FATAL: Could not create state: " << stateName << std::endl;
        }
    }

    void clearAndSwitchTo(const std::string& stateName) {
        switchTo(stateName);
    }

    void pushState(const std::string& stateName) {
        std::cout << "[StateManager] Pushing state: " << stateName << std::endl;
        
        if (!states_.empty()) {
            states_.top()->pause();
        }

        auto state = createState(stateName);
        if (state) {
            state->stateManager_ = this;
            state->onEnter();
            states_.push(std::move(state));
            std::cout << "[StateManager] Push complete (stack size: " << states_.size() << ")" << std::endl;
        } else {
            std::cerr << "[StateManager] ERROR: Failed to create state '" << stateName << "'" << std::endl;
        }
    }

    void popState() {
        if (!states_.empty()) {
            std::string stateName = "unknown"; // Would need to track this if we want to log it
            std::cout << "[StateManager] Popping state" << std::endl;
            
            states_.top()->onExit();
            states_.pop();

            if (!states_.empty()) {
                states_.top()->resume();
                std::cout << "[StateManager] Resumed underlying state (stack size: " << states_.size() << ")" << std::endl;
            } else {
                std::cout << "[StateManager] No states remaining" << std::endl;
            }
        }
    }

    void handleInput(const sf::Event& event) {
        if (!states_.empty()) {
            states_.top()->handleInput(event);
        }
    }

    void update(float deltaTime) {
        if (!states_.empty()) {
            states_.top()->update(deltaTime);
        }
    }

    void render(sf::RenderWindow& window) {
        if (!states_.empty()) {
            states_.top()->render(window);
        }
    }

    bool isEmpty() const {
        return states_.empty();
    }

    size_t getStateCount() const {
        return states_.size();
    }

private: 
    std::stack<std::unique_ptr<GameState>> states_;
    std::unordered_map<std::string, std::function<std::unique_ptr<GameState>()>> stateFactories_;
    sf::RenderWindow* window_;
    std::string currentMapPath_;

    std::unique_ptr<GameState> createState(const std::string& name) {
        auto it = stateFactories_.find(name);
        if (it != stateFactories_.end()) {
            return it->second();
        }
        
        std::cerr << "[StateManager] ERROR: State '" << name << "' not registered!" << std::endl;
        return nullptr;
    }
};