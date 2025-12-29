#include "../include/Utils/settings.h"
#include "../include/Core/StateManager.h"
#include "../include/States/MenuState.h"
#include "../include/States/PlayState.h"
#include "../include/States/PauseState.h"
#include <iostream>

int main() {
    // Create window
    sf::RenderWindow window(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Voxet");
    window.setFramerateLimit(TARGET_FPS);
    
    // Create state manager
    StateManager stateManager;
    stateManager.setWindow(&window);  // НОВОЕ: передаем окно
    
    // Register all game states
    stateManager.registerState<MenuState>("Menu");
    stateManager.registerState<PlayState>("Play");
    stateManager.registerState<PauseState>("Pause");
    
    // Start with menu state
    stateManager.pushState("Menu");
    
    std::cout << "=== Game Started ===" << std::endl;
    
    // Game clock
    sf::Clock clock;
    
    // Main game loop
    while (window.isOpen()) {
        // Exit if no states (user closed all menus)
        if (stateManager.isEmpty()) {
            std::cout << "=== No active states - Exiting ===" << std::endl;
            window.close();
            break;
        }
        
        // Handle events
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            // Forward input to current state
            stateManager.handleInput(*event);
        }
        
        // Update current state
        float deltaTime = clock.restart().asSeconds();
        stateManager.update(deltaTime);
        
        // Render current state
        window.clear(sf::Color::Black);
        stateManager.render(window);
        window.display();
    }
    
    std::cout << "=== Game Ended ===" << std::endl;
    return 0;
}