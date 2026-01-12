#pragma once
#include <SFML/Graphics.hpp>
#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <algorithm>

class GameSettings {
public:
    struct Resolution {
        unsigned int width;
        unsigned int height;
        
        std::string toString() const {
            return std::to_string(width) + "x" + std::to_string(height);
        }
        
        bool operator==(const Resolution& other) const {
            return width == other.width && height == other.height;
        }
    };

    static GameSettings& getInstance() {
        static GameSettings instance;
        return instance;
    }

    GameSettings(const GameSettings&) = delete;
    GameSettings& operator=(const GameSettings&) = delete;

    const std::vector<Resolution>& getAvailableResolutions() const { return availableResolutions_; }
    Resolution getCurrentResolution() const { return currentResolution_; }
    int getCurrentResolutionIndex() const {
        for (size_t i = 0; i < availableResolutions_.size(); ++i) {
            if (availableResolutions_[i] == currentResolution_) {
                return static_cast<int>(i);
            }
        }
        return 0;
    }
    
    void setResolution(const Resolution& res) {
        currentResolution_ = res;
        settingsChanged_ = true;
    }
    
    void setResolutionByIndex(int index) {
        if (index >= 0 && index < static_cast<int>(availableResolutions_.size())) {
            currentResolution_ = availableResolutions_[index];
            settingsChanged_ = true;
        }
    }

    bool isFullscreen() const { return fullscreen_; }
    void setFullscreen(bool fs) { 
        fullscreen_ = fs; 
        settingsChanged_ = true;
    }
    void toggleFullscreen() { setFullscreen(!fullscreen_); }

    bool isVSyncEnabled() const { return vsync_; }
    void setVSync(bool enabled) { 
        vsync_ = enabled; 
        settingsChanged_ = true;
    }
    void toggleVSync() { setVSync(!vsync_); }

    int getFPSLimit() const { return fpsLimit_; }
    void setFPSLimit(int fps) { 
        fpsLimit_ = std::clamp(fps, 30, 240); 
        settingsChanged_ = true;
    }

    float getMouseSensitivity() const { return mouseSensitivity_; }
    void setMouseSensitivity(float sens) { 
        mouseSensitivity_ = std::clamp(sens, 0.1f, 3.0f); 
        settingsChanged_ = true;
    }

    float getMasterVolume() const { return masterVolume_; }
    void setMasterVolume(float vol) { 
        masterVolume_ = std::clamp(vol, 0.0f, 1.0f); 
        settingsChanged_ = true;
    }
    
    float getMusicVolume() const { return musicVolume_; }
    void setMusicVolume(float vol) { 
        musicVolume_ = std::clamp(vol, 0.0f, 1.0f); 
        settingsChanged_ = true;
    }
    
    float getSFXVolume() const { return sfxVolume_; }
    void setSFXVolume(float vol) { 
        sfxVolume_ = std::clamp(vol, 0.0f, 1.0f); 
        settingsChanged_ = true;
    }

    float getFOV() const { return fov_; }
    void setFOV(float fov) { 
        fov_ = std::clamp(fov, 45.0f, 120.0f); 
        settingsChanged_ = true;
    }

    bool hasSettingsChanged() const { return settingsChanged_; }
    void clearSettingsChanged() { settingsChanged_ = false; }

    void applyToWindow(sf::RenderWindow& window) {
        std::cout << "[GameSettings] Applying settings..." << std::endl;
        
        // Recreate window with new settings
        sf::VideoMode mode({currentResolution_.width, currentResolution_.height});
        
        if (fullscreen_) {
            window.create(mode, "Voxet", sf::State::Fullscreen);
        } else {
            window.create(mode, "Voxet", sf::Style::Titlebar | sf::Style::Close, sf::State::Windowed);
        }
        
        window.setVerticalSyncEnabled(vsync_);
        if (!vsync_) {
            window.setFramerateLimit(fpsLimit_);
        }
        
        settingsChanged_ = false;
        
        std::cout << "[GameSettings] Applied: " << currentResolution_.toString() 
                  << " Fullscreen: " << (fullscreen_ ? "Yes" : "No")
                  << " VSync: " << (vsync_ ? "Yes" : "No") << std::endl;
    }

    bool saveSettings(const std::string& filepath = "settings.cfg") {
        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[GameSettings] ERROR: Could not save settings to " << filepath << std::endl;
            return false;
        }

        file << "# Voxet Game Settings\n";
        file << "resolution_width=" << currentResolution_.width << "\n";
        file << "resolution_height=" << currentResolution_.height << "\n";
        file << "fullscreen=" << (fullscreen_ ? "1" : "0") << "\n";
        file << "vsync=" << (vsync_ ? "1" : "0") << "\n";
        file << "fps_limit=" << fpsLimit_ << "\n";
        file << "mouse_sensitivity=" << mouseSensitivity_ << "\n";
        file << "master_volume=" << masterVolume_ << "\n";
        file << "music_volume=" << musicVolume_ << "\n";
        file << "sfx_volume=" << sfxVolume_ << "\n";
        file << "fov=" << fov_ << "\n";

        file.close();
        std::cout << "[GameSettings] Settings saved to " << filepath << std::endl;
        return true;
    }

    bool loadSettings(const std::string& filepath = "settings.cfg") {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cout << "[GameSettings] No settings file found, using defaults" << std::endl;
            return false;
        }

        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue;
            
            size_t pos = line.find('=');
            if (pos == std::string::npos) continue;
            
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);

            if (key == "resolution_width") currentResolution_.width = std::stoul(value);
            else if (key == "resolution_height") currentResolution_.height = std::stoul(value);
            else if (key == "fullscreen") fullscreen_ = (value == "1");
            else if (key == "vsync") vsync_ = (value == "1");
            else if (key == "fps_limit") fpsLimit_ = std::stoi(value);
            else if (key == "mouse_sensitivity") mouseSensitivity_ = std::stof(value);
            else if (key == "master_volume") masterVolume_ = std::stof(value);
            else if (key == "music_volume") musicVolume_ = std::stof(value);
            else if (key == "sfx_volume") sfxVolume_ = std::stof(value);
            else if (key == "fov") fov_ = std::stof(value);
        }

        file.close();
        std::cout << "[GameSettings] Settings loaded from " << filepath << std::endl;
        return true;
    }

private:
    GameSettings() {
        availableResolutions_ = {
            {1280, 720},
            {1366, 768},
            {1600, 900},
            {1920, 1080},
            {1920, 1200},
            {2560, 1440},
            {3840, 2160}
        };
        
        currentResolution_ = {1920, 1080};
        fullscreen_ = false;
        vsync_ = true;
        fpsLimit_ = 60;
        mouseSensitivity_ = 1.0f;
        masterVolume_ = 1.0f;
        musicVolume_ = 0.7f;
        sfxVolume_ = 1.0f;
        fov_ = 60.0f;
        settingsChanged_ = false;
        
        std::cout << "[GameSettings] Initialized with defaults" << std::endl;
    }

    std::vector<Resolution> availableResolutions_;
    Resolution currentResolution_;
    bool fullscreen_;
    bool vsync_;
    int fpsLimit_;
    float mouseSensitivity_;
    float masterVolume_;
    float musicVolume_;
    float sfxVolume_;
    float fov_;
    bool settingsChanged_;
};
