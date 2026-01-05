#include "../include/Utils/settings.h"
#include "../include/Core/StateManager.h"
#include "../include/Core/GameSettings.h"
#include "../include/States/MenuState.h"
#include "../include/States/PlayState_Sector.h"
#include "../include/States/PauseState.h"
#include "../include/States/OptionsState.h"
#include <iostream>

int main() {
    // Load game settings
    auto& settings = GameSettings::getInstance();
    settings.loadSettings();
    
    // Create window with settings
    auto resolution = settings.getCurrentResolution();
    sf::RenderWindow window;
    
    if (settings.isFullscreen()) {
        window.create(sf::VideoMode({resolution.width, resolution.height}), "Voxet", sf::State::Fullscreen);
    } else {
        window.create(sf::VideoMode({resolution.width, resolution.height}), "Voxet", 
                      sf::Style::Titlebar | sf::Style::Close, sf::State::Windowed);
    }
    
    // Apply VSync or FPS limit
    if (settings.isVSyncEnabled()) {
        window.setVerticalSyncEnabled(true);
    } else {
        window.setFramerateLimit(settings.getFPSLimit());
    }
    
    // Create state manager
    StateManager stateManager;
    stateManager.setWindow(&window);
    
    // Register all game states
    stateManager.registerState<MenuState>("Menu");
    stateManager.registerState<PlayState>("Play");
    stateManager.registerState<PauseState>("Pause");
    stateManager.registerState<OptionsState>("Options");
    
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
            
            // ввод в состояния
            stateManager.handleInput(*event);
        }
        
       // состояния игры от времени
        float deltaTime = clock.restart().asSeconds();
        stateManager.update(deltaTime);
        
       // рендер 
        window.clear(sf::Color::Black);
        stateManager.render(window);
        window.display();
    }
    
    std::cout << "=== Game Ended ===" << std::endl;
    return 0;
}