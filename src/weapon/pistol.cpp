#include "../../include/weapon/pistol.h"
#include "../../include/Core/ResourceManager.h"
#include <iostream>
#include <cstdio>
#include <cmath>

Gun::Gun(const std::string& basePath, int frameCount) {
    std::vector<std::string> paths;
    paths.reserve(frameCount);
    
    for (int i = 1; i <= frameCount; ++i) {
        char buffer[256];
        snprintf(buffer, sizeof(buffer), "%s%04d.png", basePath.c_str(), i);
        paths.push_back(std::string(buffer));
    }
    
    loadFrames(paths);
    loadSound("assets/sounds/pistol.mp3");
}

Gun::Gun(const std::vector<std::string>& framePaths) {
    loadFrames(framePaths);
    loadSound("assets/sounds/pistol.mp3");
}

void Gun::loadSound(const std::string& path) {
    if (auto* buffer = ResourceManager::getInstance().getSoundBuffer(path)) {
        shootSoundBuffer_ = *buffer;
        std::cout << "[Gun] Loaded sound: " << path << " (" << shootSoundBuffer_.getDuration().asSeconds() << "s)" << std::endl;
    } else {
        std::cerr << "[Gun] Failed to load sound: " << path << std::endl;
    }
}

void Gun::loadFrames(const std::vector<std::string>& paths) {
    // Загружаем все кадры анимации
    for (const auto& path : paths) {
        auto* texture = ResourceManager::getInstance().getTexture(path);
        if (!texture) {
            std::cerr << "[Gun] Failed to load frame: " << path << std::endl;
        }
        frames_.push_back(texture);
    }
    
    // Инициализируем спрайт с первым кадром
    if (!frames_.empty() && frames_[0]) {
        sprite_ = std::make_unique<sf::Sprite>(*frames_[0]);
    }
    
    std::cout << "[Gun] Loaded " << frames_.size() << " animation frames" << std::endl;
}

void Gun::update(float deltaTime, bool isMoving, bool isSprinting, 
                 float mouseDeltaX, float mouseDeltaY) {
    updateAnimation(deltaTime);
    updateBob(deltaTime, isMoving, isSprinting);
    updateSway(deltaTime, mouseDeltaX, mouseDeltaY);
    updateSounds(deltaTime);
}

void Gun::updateSounds(float deltaTime) {
    auto it = activeSounds_.begin();
    while (it != activeSounds_.end()) {
        it->second += deltaTime; // Increment time
        
        // Remove if stopped or exceeds 2 seconds (as requested)
        if (it->first.getStatus() == sf::Sound::Status::Stopped || it->second > 2.0f) {
            it = activeSounds_.erase(it);
        } else {
            ++it;
        }
    }
}

void Gun::updateBob(float deltaTime, bool isMoving, bool isSprinting) {
    if (!bobEnabled_) {
        currentBobX_ = 0.0f;
        currentBobY_ = 0.0f;
        return;
    }
    
    if (isMoving) {
        // Увеличиваем частоту при спринте
        float freq = isSprinting ? bobFrequency_ * 1.3f : bobFrequency_;
        float ampMultiplier = isSprinting ? 1.4f : 1.0f;
        
        bobPhase_ += deltaTime * freq;
        
        // Синусоидальное покачивание
        currentBobX_ = std::sin(bobPhase_) * bobAmplitudeX_ * ampMultiplier;
        currentBobY_ = std::abs(std::sin(bobPhase_ * 2.0f)) * bobAmplitudeY_ * ampMultiplier;
    } else {
        // Плавное возвращение в исходное положение
        currentBobX_ *= 0.85f;
        currentBobY_ *= 0.85f;
        bobPhase_ = 0.0f;
    }
}

void Gun::updateSway(float deltaTime, float mouseDeltaX, float mouseDeltaY) {
    if (!swayEnabled_) {
        currentSwayX_ = 0.0f;
        currentSwayY_ = 0.0f;
        return;
    }
    
    // Целевое смещение: отрицательное, т.к. пистолет смещается в противоположную сторону
    // mouseDeltaX > 0 (поворот вправо) -> пистолет влево (отрицательное смещение)
    targetSwayX_ = -mouseDeltaX * swayAmountX_;
    targetSwayY_ = -mouseDeltaY * swayAmountY_;
    
    // Ограничиваем максимальное смещение
    const float maxSway = 50.0f;
    targetSwayX_ = std::max(-maxSway, std::min(maxSway, targetSwayX_));
    targetSwayY_ = std::max(-maxSway, std::min(maxSway, targetSwayY_));
    
    // Плавная интерполяция к цели
    float lerpFactor = 1.0f - std::exp(-swaySmoothing_ * deltaTime);
    currentSwayX_ += (targetSwayX_ - currentSwayX_) * lerpFactor;
    currentSwayY_ += (targetSwayY_ - currentSwayY_) * lerpFactor;
    
    // Плавное возвращение цели к нулю
    targetSwayX_ *= 0.5f;
    targetSwayY_ *= 0.5f;
}

void Gun::updateAnimation(float deltaTime) {
    if (frames_.empty() || !sprite_) return;
    
    if (state_ == GunState::Shooting) {
        animationTimer_ += deltaTime;
        
        // Вычисляем текущий кадр на основе времени
        size_t frameIndex = static_cast<size_t>(animationTimer_ / frameDuration_);
        
        if (frameIndex >= frames_.size()) {
            // Анимация закончилась
            state_ = GunState::Idle;
            currentFrame_ = 0;
            animationTimer_ = 0.f;
            if (frames_[0]) {
                sprite_->setTexture(*frames_[0]);
            }
        } else {
            currentFrame_ = frameIndex;
            if (frames_[currentFrame_]) {
                sprite_->setTexture(*frames_[currentFrame_]);
            }
        }
    }
}

void Gun::shoot() {
    if (state_ == GunState::Idle) {
        state_ = GunState::Shooting;
        animationTimer_ = 0.f;
        currentFrame_ = 0;
        
        // Play sound
        if (shootSoundBuffer_.getSampleCount() > 0) {
            activeSounds_.emplace_back(sf::Sound(shootSoundBuffer_), 0.0f);
            activeSounds_.back().first.play();
        }
    }
}

void Gun::render(sf::RenderTarget& target, float screenWidth, float screenHeight) {
    if (frames_.empty() || !frames_[currentFrame_] || !sprite_) return;
    
    // Позиционируем пистолет в центре-снизу экрана
    auto bounds = sprite_->getLocalBounds();
    sprite_->setScale({scale_, scale_});
    
    float scaledWidth = bounds.size.x * scale_;
    float scaledHeight = bounds.size.y * scale_;
    
    // Базовая позиция: центр-низ + пользовательский offset + тряска + инерция
    float posX = (screenWidth - scaledWidth) / 2.f + offsetX_ + currentBobX_ + currentSwayX_;
    float posY = screenHeight - scaledHeight + offsetY_ - currentBobY_ + currentSwayY_;
    
    sprite_->setPosition({posX, posY});
    target.draw(*sprite_);
}