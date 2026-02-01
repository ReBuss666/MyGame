#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <iostream>
#include <optional>
#include <algorithm>
#include <cstdint>
#include "../Core/ResourceManager.h"

class Button {
public:
    Button(const std::string& text, sf::Vector2f position, sf::Vector2f size = {300.f, 60.f}) 
        : text_(text)
        , position_(position)
        , size_(size)
        , isHovered_(false)
        , font_(nullptr)
        , alpha_(255)
    {
        shape_.setSize(size_);
        shape_.setPosition(position_ - size_ / 2.f);
        shape_.setFillColor(sf::Color(50, 50, 50, 200));
        shape_.setOutlineThickness(2.f);
        shape_.setOutlineColor(sf::Color(150, 150, 150));
        
        const std::vector<std::string> fontPaths = {
            "assets/fonts/Orbitron-Black.ttf",
            "C:/Windows/Fonts/arial.ttf",
            "C:/Windows/Fonts/calibri.ttf",
            "C:/Windows/Fonts/segoeui.ttf"
        };

        for (const auto& path : fontPaths) {
            font_ = ResourceManager::getInstance().getFont(path);
            if (font_) {
                std::cout << "[Button] Font loaded from ResourceManager: " << path << std::endl;
                break;
            }
        }

        if (font_) {
            textLabel_ = sf::Text(*font_);
            textLabel_->setString(text_);
            textLabel_->setCharacterSize(24);
            textLabel_->setFillColor(sf::Color::White);
            
            sf::FloatRect textBounds = textLabel_->getLocalBounds();
            textLabel_->setOrigin({textBounds.size.x / 2.f, textBounds.size.y / 2.f});
            textLabel_->setPosition(position_);
        } else {
            std::cerr << "[Button] Warning: Could not load any font for button '" << text_ << "'" << std::endl;
        }
    }
    
    void setAlpha(float alpha) {
        alpha_ = static_cast<std::uint8_t>(std::clamp(alpha, 0.f, 255.f));
        updateColors();
    }
    
    void update(sf::Vector2f mousePos, float deltaTime = 0.f) {
        auto bounds = shape_.getGlobalBounds();
        isHovered_ = bounds.contains(mousePos);
        updateColors();
    }
    
    bool isClicked(sf::Vector2f mousePos) const {
        return shape_.getGlobalBounds().contains(mousePos);
    }
    
    void render(sf::RenderWindow& window) {
        window.draw(shape_);
        if (font_ && textLabel_) {
            window.draw(*textLabel_);
        }
    }
    
    const std::string& getName() const { return text_; }

private:
    std::string text_;
    sf::Font* font_;
    std::optional<sf::Text> textLabel_;
    sf::Vector2f position_;
    sf::Vector2f size_;
    sf::RectangleShape shape_;
    bool isHovered_;
    std::uint8_t alpha_;
    
    void updateColors() {
        if (isHovered_) {
            shape_.setFillColor(sf::Color(100, 100, 100, std::min(static_cast<std::uint8_t>(220), alpha_)));
            shape_.setOutlineColor(sf::Color(255, 255, 255, alpha_));
        } else {
            shape_.setFillColor(sf::Color(50, 50, 50, std::min(static_cast<std::uint8_t>(200), alpha_)));
            shape_.setOutlineColor(sf::Color(150, 150, 150, alpha_));
        }
        
        if (textLabel_) {
            textLabel_->setFillColor(sf::Color(255, 255, 255, alpha_));
        }
    }
};