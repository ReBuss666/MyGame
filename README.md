Voxet/
├── assets/
│   ├── fonts/
│   │   └── Orbitron-Black.ttf
│   ├── logo.png
│   └── textures/
│       └── wall.png (опционально для текстурированной версии)
│
├── include/
│   ├── Core/
│   │   ├── GameState.h          [существует]
│   │   ├── StateManager.h       [существует]
│   │   └── ResourceManager.h    [существует]
│   │
│   ├── States/
│   │   ├── MenuState.h          [существует]
│   │   ├── PauseState.h         [существует]
│   │   └── PlayState.h          [ЗАМЕНИТЬ]
│   │
│   ├── Rendering/
│   │   ├── FireEffect.h         [существует]
│   │   └── RaycasterRenderer.h  [НОВЫЙ]
│   │
│   ├── World/
│   │   └── Map.h                [существует]
│   │
│   ├── Entities/
│   │   └── player.h             [ЗАМЕНИТЬ]
│   │
│   ├── UI/
│   │   └── Button.h             [существует]
│   │
│   └── Utils/
│       └── settings.h           [ЗАМЕНИТЬ]
│
└── src/
    ├── main.cpp                 [существует]
    └── FireEffect.cpp           [существует]


// не работает exit  в самой игре а еще я пииздец жирный