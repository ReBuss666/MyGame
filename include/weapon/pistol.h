#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <vector>
#include <string>
#include <list>

enum class GunState { Idle, Shooting };

class Gun {
public:
    // Конструктор принимает базовый путь к папке с кадрами и количество кадров
    Gun(const std::string& basePath, int frameCount = 15);
    // Альтернативный конструктор со списком путей
    Gun(const std::vector<std::string>& framePaths);
    
    void update(float deltaTime, bool isMoving = false, bool isSprinting = false, 
                float mouseDeltaX = 0.0f, float mouseDeltaY = 0.0f);
    void shoot();
    void render(sf::RenderTarget& target, float screenWidth, float screenHeight);
    
    bool isAnimating() const { return state_ == GunState::Shooting; }
    
    // Настройки позиции и масштаба
    void setScale(float scale) { scale_ = scale; }
    void setOffset(float x, float y) { offsetX_ = x; offsetY_ = y; }
    void setOffsetX(float x) { offsetX_ = x; }
    void setOffsetY(float y) { offsetY_ = y; }
    float getOffsetX() const { return offsetX_; }
    float getOffsetY() const { return offsetY_; }
    
    // Настройки тряски при ходьбе
    void setBobEnabled(bool enabled) { bobEnabled_ = enabled; }
    void setBobAmplitude(float x, float y) { bobAmplitudeX_ = x; bobAmplitudeY_ = y; }
    void setBobFrequency(float freq) { bobFrequency_ = freq; }
    
    // Настройки инерции от поворота мыши (sway)
    void setSwayEnabled(bool enabled) { swayEnabled_ = enabled; }
    void setSwayAmount(float x, float y) { swayAmountX_ = x; swayAmountY_ = y; }
    void setSwaySmoothing(float smooth) { swaySmoothing_ = smooth; }

private:
    std::vector<sf::Texture*> frames_;
    std::unique_ptr<sf::Sprite> sprite_;

    // Audio
    sf::SoundBuffer shootSoundBuffer_;
    // Store sound and its playback time to enforce the 2 second limit
    std::list<std::pair<sf::Sound, float>> activeSounds_;
    void loadSound(const std::string& path);
    void updateSounds(float deltaTime);
    
    GunState state_ = GunState::Idle;
    size_t currentFrame_ = 0;
    float animationTimer_ = 0.0f;
    float frameDuration_ = 0.03f; // Скорость анимации выстрела
    float scale_ = 4.0f;
    
    // Позиция (смещение от центра-низа)
    float offsetX_ = -5.0f;   // + вправо, - влево
    float offsetY_ = -2.0f;   // + вниз, - вверх
    
    // Тряска при ходьбе (weapon bob)
    bool bobEnabled_ = true;
    float bobPhase_ = 0.0f;
    float bobAmplitudeX_ = 8.0f;   // Горизонтальная амплитуда
    float bobAmplitudeY_ = 12.0f;  // Вертикальная амплитуда
    float bobFrequency_ = 5.0f;   // Частота покачивания
    float currentBobX_ = 0.0f;
    float currentBobY_ = 0.0f;
    
    // Инерция от поворота мыши (weapon sway)
    bool swayEnabled_ = true;
    float swayAmountX_ = 15.0f;    // Множитель смещения по X (пистолет влево при повороте вправо)
    float swayAmountY_ = 6.0f;     // Множитель смещения по Y (пистолет вверх при взгляде вниз)
    float swaySmoothing_ = 8.0f;   // Скорость возврата (чем больше - тем быстрее)
    float currentSwayX_ = 0.0f;
    float currentSwayY_ = 0.0f;
    float targetSwayX_ = 0.0f;
    float targetSwayY_ = 0.0f;
    
    void loadFrames(const std::vector<std::string>& paths);
    void updateAnimation(float deltaTime);
    void updateBob(float deltaTime, bool isMoving, bool isSprinting);
    void updateSway(float deltaTime, float mouseDeltaX, float mouseDeltaY);
};