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

## 🎯 План Реализации (Пошагово)

### **Фаза 1: Базовые Структуры Данных** (1-2 дня)
- [ ] `Sector.h/cpp` - базовый класс сектора
- [ ] `Wall.h/cpp` - класс стены с порталами
- [ ] `SectorMap.h/cpp` - контейнер секторов
- [ ] Простая тестовая карта (2-3 сектора)

### **Фаза 2: Базовый Рендеринг** (2-3 дня)
- [ ] `SectorRenderer.h/cpp` - рендеринг одного сектора
- [ ] Рендеринг стен без порталов
- [ ] Рендеринг пола и потолка
- [ ] Интеграция в `PlayState`

### **Фаза 3: Портальная Система** (3-4 дня)
- [ ] `PortalRenderer.h/cpp` - рекурсивный рендеринг порталов
- [ ] Клиппинг (frustum culling)
- [ ] Обработка разных высот полов
- [ ] Обработка разных высот потолков

### **Фаза 4: Текстурирование** (2-3 дня)
- [ ] `TextureAtlas.h` - управление текстурами
- [ ] UV-mapping для стен
- [ ] Текстуры полов/потолков
- [ ] Aligned vs unaligned текстуры

### **Фаза 5: Загрузка Карт** (2 дня)
- [ ] `MapLoader.h/cpp` - парсер JSON
- [ ] Формат карты (JSON)
- [ ] Валидация геометрии
- [ ] Тестовые карты со ступеньками

### **Фаза 6: Физика и Коллизии** (2-3 дня)
- [ ] Обновление `Player` для высоты
- [ ] Коллизии со стенами (line-segment)
- [ ] Ступеньки (автоподъем)
- [ ] Гравитация и падение

### **Фаза 7: Оптимизация** (1-2 дня)
- [ ] BSP-дерево (опционально)
- [ ] Frustum culling
- [ ] Occlusion culling
- [ ] Батчинг вершин

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

## 🚀 Git Workflow

### Создание Ветки

```bash
# Создать новую ветку для разработки
git checkout -b feature/sector-engine

# Коммитить маленькими шагами
git commit -m "Add Sector and Wall classes"
git commit -m "Implement basic SectorRenderer"
git commit -m "Add portal rendering"
```

### Тестирование

- Держите `main` ветку стабильной
- Тестируйте каждую фазу перед переходом к следующей
- Создайте простые тест-карты для каждой фичи

---

## 📊 Преимущества Нового Движка

| Фича | Grid-Based | Sector-Based |
|------|-----------|--------------|
| Ступеньки | ❌ | ✅ |
| Окна | ❌ | ✅ |
| Разная высота потолков | ❌ | ✅ |
| Наклонные стены | ❌ | ✅ |
| Сложная геометрия | ❌ | ✅ |
| Двери | 🟡 Сложно | ✅ Легко |
| Производительность | 🟡 | ✅ Лучше |

---

## 🔍 Полезные Ресурсы

1. **Doom Engine Black Book** - Fabien Sanglard
2. **Doom Wiki**: https://doomwiki.org/wiki/Doom_rendering_engine
3. **Bisqwit's Portal Rendering**: https://youtu.be/HQYsFshbkYw
4. **Build Engine Documentation**

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

## 🎯 Цели MVP (Minimum Viable Product)

Для первой рабочей версии нужно:
- ✅ 2-3 сектора с разной высотой полов
- ✅ Ступенька вверх и вниз
- ✅ Окно (разная высота потолков)
- ✅ Текстуры стен
- ✅ Коллизии и движение игрока

---

## 📝 Следующие Шаги

1. **Создать Git ветку**: `git checkout -b feature/sector-engine`
2. **Начать с Фазы 1**: Sector.h, Wall.h, SectorMap.h
3. **Написать простой тест**: 2 сектора с порталом между ними
4. **Постепенно добавлять фичи**: не пытайтесь сделать все сразу!

Хотите, чтобы я начал с конкретной имплементации первых классов?