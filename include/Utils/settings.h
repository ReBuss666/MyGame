#pragma once
#include <cmath>

// =============================================================================
// MATH CONSTANTS
// =============================================================================
#ifndef M_PI
    #define M_PI 3.14159265358979323846
#endif

// =============================================================================
// WINDOW SETTINGS
// =============================================================================
inline constexpr int WINDOW_WIDTH = 1920;
inline constexpr int WINDOW_HEIGHT = 1200;
inline constexpr float WINDOW_CENTER_X = WINDOW_WIDTH / 2.0f;
inline constexpr float WINDOW_CENTER_Y = WINDOW_HEIGHT / 2.0f;

// =============================================================================
// RENDERING SETTINGS
// =============================================================================
inline constexpr int TARGET_FPS = 60;
inline constexpr float FPS_UPDATE_INTERVAL = 0.3f;

// =============================================================================
// RAYCASTER SETTINGS (3D MODE)
// =============================================================================

// field of view
inline constexpr float FOV_DEGREES = 60.0f;
inline constexpr float FOV_RADIANS = FOV_DEGREES * 3.14159265358979323846f / 180.0f;

// Render distance
inline constexpr float RAYCASTER_RENDER_DISTANCE = 20.0f;

// Wall height scaling
inline constexpr float RAYCASTER_WALL_HEIGHT = 64.0f;

// Sky/Floor colors
inline constexpr unsigned char SKY_R = 70;
inline constexpr unsigned char SKY_G = 130;
inline constexpr unsigned char SKY_B = 180;

inline constexpr unsigned char FLOOR_R = 50;
inline constexpr unsigned char FLOOR_G = 50;
inline constexpr unsigned char FLOOR_B = 50;

// Mouse sensitivity (3D Mode)
inline constexpr float MOUSE_SENSITIVITY = 0.002f;

//Keyboard rotation speed (3D Mode)
inline constexpr float KEYBOARD_ROTATION_SPEED = 2.5f; // rad per sec

// =============================================================================
// FIRE EFFECT SETTINGS
// =============================================================================
inline constexpr int FIRE_HEIGHT = 300;
inline constexpr int FIRE_WIDTH = 480; 
inline constexpr int PixelSize = WINDOW_WIDTH / FIRE_WIDTH;
inline constexpr int FLASH_DURATION = 180;

// =============================================================================
// MENU/UI SETTINGS
// =============================================================================
// Logo Animation
inline constexpr float LOGO_SPEED = 120.0f;
inline constexpr float LOGO_OFFSET_Y = 60.0f;

// Menu Layout
inline constexpr float MENU_BUTTON_START_Y = 650.0f;
inline constexpr float MENU_BUTTON_SPACING = 100.0f;
inline constexpr float MENU_BUTTON_WIDTH = 300.0f;
inline constexpr float MENU_BUTTON_HEIGHT = 60.0f;

// =============================================================================
// PLAYER SETTINGS
// =============================================================================
// Movement
inline constexpr float PLAYER_SPEED = 600.0f;
inline constexpr float PLAYER_ACCELERATION = 2000.0f;
inline constexpr float PLAYER_FRICTION = 10.0f;

// Size
inline constexpr float PLAYER_WIDTH = 40.0f;
inline constexpr float PLAYER_HEIGHT = 40.0f;

// Collision
inline constexpr float COLLISION_BUFFER = 1.0f;

// View (3D-Mode)
inline constexpr float PLAYER_VIEW_HEIGHT = 32.0f;

// =============================================================================
// MAP SETTINGS
// =============================================================================
inline constexpr float TILE_SIZE = 64.0f;
inline constexpr float TILE_PADDING = 2.0f; // Visual gap between tiles

// =============================================================================
// GAME CONSTANTS
// =============================================================================
inline constexpr float GRAVITY = 980.0f; // Pixels per second squared (if needed)
inline constexpr float MAX_FALL_SPEED = 1000.0f; // Maximum falling speed (if needed)

// =============================================================================
// ASSET PATHS
// =============================================================================
namespace Assets {
    // Textures
    inline constexpr const char* LOGO_TEXTURE = "../assets/logo.png";
    
    // Fonts
    inline constexpr const char* FONT_PRIMARY = "../assets/fonts/Orbitron-Black.ttf";
    inline constexpr const char* FONT_FALLBACK_1 = "C:/Windows/Fonts/arial.ttf";
    inline constexpr const char* FONT_FALLBACK_2 = "C:/Windows/Fonts/calibri.ttf";
    inline constexpr const char* FONT_FALLBACK_3 = "C:/Windows/Fonts/segoeui.ttf";
    
    // Sounds (placeholder for future use)
    inline constexpr const char* SOUND_MENU_CLICK = "../assets/sounds/menu_click.wav";
    inline constexpr const char* SOUND_BUTTON_HOVER = "../assets/sounds/button_hover.wav";
}

// =============================================================================
// DEBUG SETTINGS
// =============================================================================
#ifdef _DEBUG
    inline constexpr bool DEBUG_DRAW_COLLISION = true;
    inline constexpr bool DEBUG_SHOW_FPS = true;
    inline constexpr bool DEBUG_LOG_RESOURCE_LOADING = true;
#else
    inline constexpr bool DEBUG_DRAW_COLLISION = false;
    inline constexpr bool DEBUG_SHOW_FPS = false;
    inline constexpr bool DEBUG_LOG_RESOURCE_LOADING = false;
#endif