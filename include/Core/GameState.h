#pragma once
#include <SFML/Graphics.hpp>
#include <memory>

class StateManager;

/**
 * @brief Base class for all game states (Menu, Play, Pause, etc.)
 * 
 * All game states must inherit from this class and implement the pure virtual methods.
 * The StateManager handles transitions between states.
 */
class GameState {
public:
    virtual ~GameState() = default;

    // Called when state becomes active
    virtual void onEnter() = 0;
    
    // Called when state is being removed
    virtual void onExit() = 0;

    // Handle user input events
    virtual void handleInput(const sf::Event& event) = 0;
    
    // Update game logic
    virtual void update(float deltaTime) = 0;
    
    // Render to screen
    virtual void render(sf::RenderWindow& window) = 0;

    // Optional: called when another state is pushed on top
    virtual void pause() {}
    
    // Optional: called when state becomes active again after being paused
    virtual void resume() {}

protected:
    StateManager* stateManager_ = nullptr;
    friend class StateManager;
};