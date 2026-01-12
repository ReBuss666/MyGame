#pragma once
#include <SFML/Graphics.hpp>
#include <memory>

class StateManager;

class GameState {
public:
    virtual ~GameState() = default;

    virtual void onEnter() = 0;
    
    virtual void onExit() = 0;

    virtual void handleInput(const sf::Event& event) = 0;
    
    virtual void update(float deltaTime) = 0;
    
    virtual void render(sf::RenderWindow& window) = 0;

    virtual void pause() {}
    
    virtual void resume() {}

protected:
    StateManager* stateManager_ = nullptr;
    friend class StateManager;
};