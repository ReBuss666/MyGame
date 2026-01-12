#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <unordered_map>
#include <memory>
#include <string>
#include <iostream>

class ResourceManager {
public:
    static ResourceManager& getInstance() {
        static ResourceManager instance;
        return instance;
    }

    ResourceManager(const ResourceManager&) = delete;
    ResourceManager& operator=(const ResourceManager&) = delete;

    sf::Texture* getTexture(const std::string& filepath) {
        auto it = textures_.find(filepath);
        if (it != textures_.end()) {
            std::cout << "[ResourceManager] Texture '" << filepath << "' loaded from cache" << std::endl;
            return it->second.get();
        }

        auto texture = std::make_unique<sf::Texture>();
        if (!texture->loadFromFile(filepath)) {
            std::cerr << "[ResourceManager] Failed to load texture: " << filepath << std::endl;
            return nullptr;
        }

        std::cout << "[ResourceManager] Texture '" << filepath << "' loaded from disk" << std::endl;
        texture->setSmooth(false);
        
        sf::Texture* ptr = texture.get();
        textures_[filepath] = std::move(texture);
        return ptr;
    }

    sf::Font* getFont(const std::string& filepath) {
        auto it = fonts_.find(filepath);
        if (it != fonts_.end()) {
            std::cout << "[ResourceManager] Font '" << filepath << "' loaded from cache" << std::endl;
            return it->second.get();
        }

        auto font = std::make_unique<sf::Font>();
        if (!font->openFromFile(filepath)) {
            std::cerr << "[ResourceManager] Failed to load font: " << filepath << std::endl;
            return nullptr;
        }

        std::cout << "[ResourceManager] Font '" << filepath << "' loaded from disk" << std::endl;
        
        sf::Font* ptr = font.get();
        fonts_[filepath] = std::move(font);
        return ptr;
    }

    sf::SoundBuffer* getSoundBuffer(const std::string& filepath) {
        auto it = soundBuffers_.find(filepath);
        if (it != soundBuffers_.end()) {
            return it->second.get();
        }

        auto buffer = std::make_unique<sf::SoundBuffer>();
        if (!buffer->loadFromFile(filepath)) {
            std::cerr << "[ResourceManager] Failed to load sound: " << filepath << std::endl;
            return nullptr;
        }

        std::cout << "[ResourceManager] Sound '" << filepath << "' loaded from disk" << std::endl;
        
        sf::SoundBuffer* ptr = buffer.get();
        soundBuffers_[filepath] = std::move(buffer);
        return ptr;
    }

    void clearTextures() { 
        textures_.clear(); 
        std::cout << "[ResourceManager] All textures cleared" << std::endl;
    }
    
    void clearFonts() { 
        fonts_.clear(); 
        std::cout << "[ResourceManager] All fonts cleared" << std::endl;
    }
    
    void clearSounds() { 
        soundBuffers_.clear(); 
        std::cout << "[ResourceManager] All sounds cleared" << std::endl;
    }

    void clearAll() {
        textures_.clear();
        fonts_.clear();
        soundBuffers_.clear();
        std::cout << "[ResourceManager] All resources cleared" << std::endl;
    }

    size_t getTextureCount() const { return textures_.size(); }
    size_t getFontCount() const { return fonts_.size(); }
    size_t getSoundCount() const { return soundBuffers_.size(); }

    void printStats() const {
        std::cout << "[ResourceManager] Stats:" << std::endl;
        std::cout << "  Textures: " << textures_.size() << std::endl;
        std::cout << "  Fonts: " << fonts_.size() << std::endl;
        std::cout << "  Sounds: " << soundBuffers_.size() << std::endl;
    }

private:
    ResourceManager() {
        std::cout << "[ResourceManager] Initialized" << std::endl;
    }

    ~ResourceManager() {
        std::cout << "[ResourceManager] Shutdown" << std::endl;
    }

    std::unordered_map<std::string, std::unique_ptr<sf::Texture>> textures_;
    std::unordered_map<std::string, std::unique_ptr<sf::Font>> fonts_;
    std::unordered_map<std::string, std::unique_ptr<sf::SoundBuffer>> soundBuffers_;
};