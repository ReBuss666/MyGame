# Архитектура Секторного Движка (Doom-style)

## 📋 Обзор

Переход от grid-based raycaster к sector-based engine для поддержки:
- ✅ Разноуровневые полы и потолки
- ✅ Ступеньки и лестницы
- ✅ Окна и проемы
- ✅ Сложная геометрия карт
- ✅ Двери и подвижные платформы (в будущем)

---

## 🏗️ Основные Концепции

### 1. **Sector (Сектор)**
Замкнутый полигональный регион с:
- `floorHeight` - высота пола
- `ceilingHeight` - высота потолка
- `floorTexture` - текстура пола
- `ceilingTexture` - текстура потолка
- `lightLevel` - уровень освещения
- `walls` - список стен сектора

### 2. **Wall (Стена)**
Линия между двумя точками:
- `start` - начальная точка (x, y)
- `end` - конечная точка (x, y)
- `neighborSector` - ID соседнего сектора (nullptr = твердая стена)
- `upperTexture` - текстура над проемом
- `middleTexture` - текстура стены
- `lowerTexture` - текстура под проемом

### 3. **Portal**
Если `wall.neighborSector != nullptr`, стена является порталом:
- Можно видеть соседний сектор
- Если разница высот полов > 0 → нижняя текстура (ступенька вверх)
- Если разница высот потолков < 0 → верхняя текстура (окно)

---

## 📂 Структура Файлов

```
include/
├── World/
│   ├── Sector.h              # NEW - Класс сектора
│   ├── Wall.h                # NEW - Класс стены
│   ├── SectorMap.h           # NEW - Карта секторов (замена Map.h)
│   └── MapLoader.h           # NEW - Загрузка карт из JSON/текста
│
├── Rendering/
│   ├── SectorRenderer.h      # NEW - Рендерер секторов (замена RayCasterRenderer)
│   ├── PortalRenderer.h      # NEW - Рендеринг порталов
│   └── TextureAtlas.h        # NEW - Управление текстурами
│
└── Utils/
    └── Geometry.h            # NEW - Геометрические утилиты

src/
├── World/
│   ├── Sector.cpp
│   ├── Wall.cpp
│   ├── SectorMap.cpp
│   └── MapLoader.cpp
│
└── Rendering/
    ├── SectorRenderer.cpp
    └── PortalRenderer.cpp
```
---

## 🔧 Ключевые Алгоритмы

### 1. **Portal Rendering Algorithm**

```cpp
void renderSector(Sector& sector, FrustumClip& frustum) {
    // 1. Отрендерить все видимые стены
    for (auto& wall : sector.walls) {
        if (!frustum.isVisible(wall)) continue;
        
        if (wall.neighborSector == nullptr) {
            // Твердая стена
            renderSolidWall(wall, sector);
        } else {
            // Портал
            Sector& neighbor = *wall.neighborSector;
            
            // Рендер верхней/нижней частей
            if (neighbor.ceilingHeight < sector.ceilingHeight)
                renderUpperWall(wall, sector, neighbor);
            if (neighbor.floorHeight > sector.floorHeight)
                renderLowerWall(wall, sector, neighbor);
            
            // Рекурсивно отрендерить соседний сектор
            FrustumClip newFrustum = frustum.clipToPortal(wall);
            renderSector(neighbor, newFrustum);
        }
    }
    
    // 2. Отрендерить пол и потолок
    renderFloorCeiling(sector, frustum);
}
```
---
### 2. **Line-Segment Collision**

```cpp
bool checkWallCollision(Vector2f pos, float radius, Wall& wall) {
    // Найти ближайшую точку на линии стены
    Vector2f closest = closestPointOnSegment(pos, wall.start, wall.end);
    float dist = distance(pos, closest);
    
    if (dist < radius) {
        // Если стена - портал, проверить высоту
        if (wall.neighborSector) {
            float heightDiff = wall.neighborSector->floorHeight - currentSector->floorHeight;
            if (heightDiff <= STEP_HEIGHT) {
                // Можно пройти (ступенька)
                return false;
            }
        }
        return true; // Коллизия
    }
    return false;
}
```

---

## 📐 Формат Карты (JSON)

```json
{
  "sectors": [
    {
      "id": 0,
      "floorHeight": 0.0,
      "ceilingHeight": 2.5,
      "floorTexture": "floor_stone.png",
      "ceilingTexture": "ceiling_wood.png",
      "lightLevel": 200,
      "walls": [
        {
          "start": [0, 0],
          "end": [5, 0],
          "neighborSector": null,
          "middleTexture": "wall_brick.png"
        },
        {
          "start": [5, 0],
          "end": [5, 5],
          "neighborSector": 1,
          "upperTexture": "wall_brick.png",
          "lowerTexture": "wall_stone.png"
        }
      ]
    },
    {
      "id": 1,
      "floorHeight": 0.5,
      "ceilingHeight": 3.0,
      "walls": [...]
    }
  ],
  "playerStart": {
    "sector": 0,
    "position": [2.5, 2.5],
    "angle": 0
  }
}
```

---

## 🎮 Обновления Игровой Логики

### Player Changes

```cpp
class Player {
    // NEW
    float height;          // Высота игрока (например, 1.8м)
    float eyeHeight;       // Высота глаз (1.6м от пола)
    Sector* currentSector; // Текущий сектор
    
    // Обновленная физика
    void updatePhysics(float dt) {
        // Гравитация
        if (!onGround) {
            velocityZ -= GRAVITY * dt;
        }
        
        // Проверка коллизий со стенами
        for (auto& wall : currentSector->walls) {
            if (checkWallCollision(position, PLAYER_RADIUS, wall)) {
                // Обработка коллизии
            }
        }
        
        // Обновление текущего сектора
        updateCurrentSector();
    }
};
```
---

## ⚠️ Возможные Проблемы

1. **Сложность порталов** - Рекурсия может быть медленной
   - Решение: Ограничить глубину рекурсии (5-6 уровней)

2. **Z-fighting на полах** - Артефакты рендеринга
   - Решение: Правильный depth sorting

3. **Невыпуклые сектора** - Могут ломать рендеринг
   - Решение: Разбивать на выпуклые или использовать BSP

4. **Текстуры стен** - UV-mapping сложнее чем в grid
   - Решение: Использовать aligned текстуры по умолчанию

---