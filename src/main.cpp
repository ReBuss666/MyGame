#include "../include/Utils/settings.h"
#include "../include/Core/StateManager.h"
#include "../include/Core/GameSettings.h"
#include "../include/States/MenuState.h"
#include "../include/States/PlayState_Sector.h"
#include "../include/States/PauseState.h"
#include "../include/States/OptionsState.h"
#include <iostream>

int main() {
    auto& settings = GameSettings::getInstance();
    settings.loadSettings();
    
    auto resolution = settings.getCurrentResolution();
    sf::RenderWindow window;
    
    if (settings.isFullscreen()) {
        window.create(sf::VideoMode({resolution.width, resolution.height}), "Voxet", sf::State::Fullscreen);
    } else {
        window.create(sf::VideoMode({resolution.width, resolution.height}), "Voxet", 
                      sf::Style::Titlebar | sf::Style::Close, sf::State::Windowed);
    }
    
    if (settings.isVSyncEnabled()) {
        window.setVerticalSyncEnabled(true);
    } else {
        window.setFramerateLimit(settings.getFPSLimit());
    }
    
    StateManager stateManager;
    stateManager.setWindow(&window);
    
    stateManager.registerState<MenuState>("Menu");
    stateManager.registerState<PlayState>("Play");
    stateManager.registerState<PauseState>("Pause");
    stateManager.registerState<OptionsState>("Options");
    
    stateManager.pushState("Menu");
    
    std::cout << "=== Game Started ===" << std::endl;
    
    sf::Clock clock;
    
    while (window.isOpen()) {
        if (stateManager.isEmpty()) {
            std::cout << "=== No active states - Exiting ===" << std::endl;
            window.close();
            break;
        }
        
        while (auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
            
            stateManager.handleInput(*event);
        }
        
        float deltaTime = clock.restart().asSeconds();
        stateManager.update(deltaTime);
        
        window.clear(sf::Color::Black);
        stateManager.render(window);
        window.display();
    }
    
    std::cout << "=== Game Ended ===" << std::endl;
    return 0;
}