# Roadmap: Dungeon Crawlers — от прототипа к Eye of Beholder

> Основа: скелет 3D-движка на OpenGL 3.3 + SDL3 + ImGui, ~3500 строк
> Цель: партийная пошаговая RPG в ретро-стилистике (Eye of Beholder)
> Графика: 3D-сцена (OpenGL 3.3 core) с текстурированными quad'ами, биллбордами, instancing
> Ретро-стиль: низкое разрешение FBO (320×240) + nearest-neighbour фильтр

---

##  Условные обозначения

- `[Engine]` — изменения в движке (renderer, core, engine/)
- `[Game]` — изменения в игровой логике (game/)
- `[Content]` — создание ассетов (текстуры, звуки, данные)
- `[Build]` — изменения сборки (build.bat, vcxproj)
- `[Test]` — написание тестов

---

# Фаза 1: Одногеройный roguelike-прототип (шаги 1–60)

> Цель: играбельный прототип, где один герой ходит по подземелью, сражается с монстрами в пошаговом режиме (как классический roguelike), собирает предметы.

## Этап 1.1: Переход от FPS-камеры к грид-системе (шаги 1–10)

### Шаг 1: Специальный `GridPosition`

```
src/game/grid_position.h — новая структура
```

```cpp
struct GridPosition final
{
    int32_t row = 0;
    int32_t col = 0;
    int32_t floor = 0;
};
```

Создать POD-структуру с операторами сравнения `==`, `!=`, хешированием для `unordered_map`.
Никакой логики — просто данные.

### Шаг 2: `enum class Direction`

```
src/game/direction.h — новый файл
```

```cpp
enum class Direction : uint8_t
{
    North, // -row
    East,  // +col
    South, // +row
    West   // -col
};
```

Функции:
- `DirectionFromIndex(int32_t i)` — циклическое получение
- `NextDirection(Direction d, bool clockwise)` — поворот на 90°
- `DirectionToVec(Direction d) → glm::ivec2` — смещение (row, col)
- `DirectionToYaw(Direction d) → float` — угол в градусах для камеры

Добавить `#include` в `stdafx.h`.

### Шаг 3: Расширение `Camera` в `GridCamera`

```
src/engine/renderer/camera.h — модификация
```

Добавить в класс `Camera`:
- `m_gridPosition : GridPosition` — позиция на гриде
- `m_gridFacing : Direction` — направление взгляда
- `m_targetPosition : glm::vec3` — целевая позиция для lerp
- `m_targetYaw : float` — целевой yaw для lerp
- `m_isAnimating : bool` — флаг анимации рывка
- `m_animationTimer : float` — время от начала анимации (0..ANIMATION_DURATION)

Новые методы:
- `SetGridPosition(GridPosition pos, Direction facing)` — установка позиции на гриде
- `SnapToGrid()` — мгновенный рывок без анимации
- `IsAnimating() → bool`
- `GridPosition() const → GridPosition`
- `Facing() const → Direction`
- `TurnLeft() / TurnRight() / MoveForward() / MoveBackward()` — грид-движение

Константы:
- `static constexpr float ANIMATION_DURATION = 0.2f` — длительность рывка
- `static constexpr float GRID_UNIT = 1.0f` — размер клетки

### Шаг 4: Lerp-анимация в `Camera::UpdateAnimation(float dt)`

```
src/engine/renderer/camera.cpp — новый метод
```

```cpp
void Camera::UpdateAnimation(float dt) noexcept;
```

Если `m_isAnimating`, инкрементировать `m_animationTimer`, lerp-ить `m_position` от старой к `m_targetPosition`, lerp-ить `m_yaw` от старого к `m_targetYaw`. По окончании вызвать `SnapToGrid()` — установить точные значения, выключить флаг.

Вызывать из `PlayState::Update()` перед чтением позиции камеры.

### Шаг 5: Вращение 90° за шаг

`TurnLeft()`:
1. Сохранить текущий `m_yaw` как стартовый
2. `m_gridFacing = NextDirection(m_gridFacing, false)`
3. `m_targetYaw = DirectionToYaw(m_gridFacing)`
4. `m_animationTimer = 0, m_isAnimating = true`

`TurnRight()` — аналогично с `true`.

### Шаг 6: Движение на 1 клетку

`MoveForward()`:
1. Вычислить `glm::ivec2 delta = DirectionToVec(m_gridFacing)`
2. Новая `GridPosition target(m_gridPosition.row + delta.x, m_gridPosition.col + delta.y, m_gridPosition.floor)`
3. Сохранить старую позицию
4. `m_gridPosition = target`
5. `m_targetPosition = GridToWorld(target)`
6. `m_animationTimer = 0, m_isAnimating = true`

`MoveBackward()` — аналогично с `-delta`.

`GridToWorld(GridPosition p) → glm::vec3` — конвертация: `x = p.col * GRID_UNIT, z = p.row * GRID_UNIT`.

### Шаг 7: Отключение FPS-управления в `PlayState`

```
src/game/states/play_state.cpp — модификация
```

- Удалить биндинги `MoveForward`/`MoveBackward`/`MoveLeft`/`MoveRight`/`MoveUp`/`MoveDown`
- Заменить на биндинги: `GridMoveForward` (W), `GridMoveBackward` (S), `TurnLeft` (A), `TurnRight` (D)
- `Update()` вызывает не `m_camera.MoveForward()` а `m_camera.MoveForward()` (новый grid-метод) и `m_camera.UpdateAnimation(dt)`
- Удалить mouse-look (на этом этапе камера смотрит строго по направлению)

### Шаг 8: Удаление кубов из `PlayState::OnEnter()`

```
src/game/states/play_state.cpp — очистка
```

- Убрать генерацию сетки кубов
- Убрать `NUM_MATERIALS`, `m_materials[]`, `m_cubes`, `m_debugSphere`
- Убрать коллизионную логику из `FixedUpdate`
- Оставить только `m_camera`, `m_shader`, `m_cube` (как тестовый меш)
- Добавить тестовый пол — один большой quad (4 вершины) через `Mesh::Create()`

### Шаг 9: Добавление `DungeonTile` и `Dungeon` класса (каркас)

```
src/game/dungeon/tile.h — новый модуль
src/game/dungeon/dungeon.h — новый модуль
```

```cpp
enum class TileType : uint8_t
{
    Void, Floor, Wall, DoorClosed, DoorOpen
};

struct Tile final
{
    TileType type = TileType::Void;
};

struct LevelData final
{
    std::vector<std::vector<Tile>> tiles; // [row][col]
    int32_t width = 0;
    int32_t height = 0;
};

class Dungeon final
{
public:
    void GenerateTestRoom(); // комната 8×8 с Floor и Wall по краям
    [[nodiscard]] const LevelData& CurrentLevel() const noexcept;
    [[nodiscard]] bool IsWalkable(GridPosition pos) const noexcept;
    [[nodiscard]] TileType GetTile(GridPosition pos) const noexcept;
private:
    std::vector<LevelData> m_levels;
    int32_t m_currentFloor = 0;
};
```

`GenerateTestRoom()`: создать LevelData 8×8, все клетки Floor, края — Wall.

### Шаг 10: Интеграция `Dungeon` в `PlayState` и проверка проходимости

```
src/game/states/play_state.h/.cpp — модификация
```

- Добавить `Dungeon m_dungeon` как член `PlayState`
- В `OnEnter()` вызвать `m_dungeon.GenerateTestRoom()`
- Перед `MoveForward()` проверять `m_dungeon.IsWalkable(targetPos)`
- Если стена — не двигаться (без анимации)
- Если `Void` — не двигаться (или падение, пока не реализовано)

---

## Этап 1.2: Рендер подземелья (шаги 11–20)

### Шаг 11: Генератор геометрии пола

```
src/game/dungeon/dungeon_renderer.h — новый модуль
```

Структура `DungeonGeometry`: хранит векторы `Vertex` и `uint32_t` indices для:
- Всех тайлов пола (один batch)
- Всех тайлов стен (один batch)
- Потолка (один batch)

Метод `BuildFloorGeometry(const LevelData& level)`:
- Для каждого `Floor` создать quad на высоте `y=0` размером 1×1
- Текстурные координаты: (0,0)-(1,1) на клетку
- Вернуть векторы Vertex/indices

### Шаг 12: Генератор геометрии стен

`BuildWallGeometry(const LevelData& level)`:
- Для каждой стены определить, какие стороны видимы
- Если клетка `Wall` и соседняя `Floor` — генерировать quad стены на границе
- Стена высотой 1.0 (y=0..1)
- Нормали: наружу от стены
- Текстурные координаты: (0,0)-(1,1) для каждой грани

### Шаг 13: `DungeonRenderer` — класс для управления геометрией

```
src/game/dungeon/dungeon_renderer.h/.cpp
```

```cpp
class DungeonRenderer final
{
public:
    void Build(const Dungeon& dungeon);
    void Submit(Renderer& renderer, const glm::mat4& viewProj);

private:
    Mesh m_floorMesh;
    Mesh m_wallMesh;
    Mesh m_ceilingMesh;
    Material* m_floorMaterial = nullptr;
    Material* m_wallMaterial = nullptr;
    Material* m_ceilingMaterial = nullptr;
};
```

При `Build()` заново создавать меши из сгенерированной геометрии.
При `Submit()` вызывать `renderer.Submit()` для каждого меша.

### Шаг 14: Шейдеры для dungeon

Создать шейдеры `dungeon.vert` / `dungeon.frag` (строковые константы в коде):
- Вершинный: позиция + нормаль + текстурные координаты, UBO `ViewProjection`, модель
- Фрагментный: текстура × простое ambient-освещение (нормаль·свет = 0.5 + 0.5)

Добавить uniform `uAmbientLight` (float).

### Шаг 15: Текстуры для подземелья (процедурные)

Через `ResourceManager::CreateCheckerboard()`:
- `tex_dungeon_floor` — 64×64, 8 клеток, серо-коричневые тона
- `tex_dungeon_wall` — 64×64, 8 клеток, серый камень
- `tex_dungeon_ceiling` — 64×64, 8 клеток, тёмно-серый

Позже заменить на рукописные пиксель-арт текстуры.

### Шаг 16: Материалы dungeon

```
PlayState::OnEnter() — расширение
```

Создать материалы:
- `mat_dungeon_floor` = (dungeonShader, tex_dungeon_floor)
- `mat_dungeon_wall` = (dungeonShader, tex_dungeon_wall)
- `mat_dungeon_ceiling` = (dungeonShader, tex_dungeon_ceiling)

### Шаг 17: Подключение DungeonRenderer в PlayState

```
PlayState — рендер сцены
```

В `RenderScene()`:
1. `m_dungeonRenderer.Build(m_dungeon)` — если изменилась карта
2. `renderer.BeginFrame()`
3. `m_dungeonRenderer.Submit(renderer)`
4. `renderer.EndFrame()`

Пока не оптимизируем — перестраиваем геометрию каждый кадр.

### Шаг 18: FBO с ретро-разрешением

Уже есть в `m_renderHeight` (settings). Убедиться, что FBO создаётся 320×240 (или 480×320) с `GL_NEAREST` фильтром.
Текущий код: `CreateCheckerboard` уже использует `GL_NEAREST`.

Проверить `framebuffer.cpp`: фильтр на color texture — `GL_NEAREST`. Если нет — исправить.

### Шаг 19: Позиционирование камеры по гриду

При старте игры:
- `m_camera.SetGridPosition({1, 1, 0}, Direction::North)`
- `m_camera.SnapToGrid()`

Камера находится на высоте `y=0.5` (середина клетки, уровень глаз).

### Шаг 20: Визуальная проверка

Запустить игру, проверить:
- Клетки пола видны снизу
- Стены по краям видны
- Камера движется рывками по клеткам
- Поворот на 90° за шаг
- Ретро-фильтр (пикселизация) через FBO

---

## Этап 1.3: Пошаговый режим и ввод (шаги 21–28)

### Шаг 21: `enum class GameMode`

```
src/game/game_mode.h — новый файл
```

```cpp
enum class GameMode : uint8_t
{
    Exploring,  // свободное перемещение
    TurnWaiting, // ждём действия игрока
    TurnAnimating, // проигрывается анимация
    CombatTurn,   // боевой ход
    GameOver
};
```

### Шаг 22: Конечный автомат ходов в `PlayState`

```
PlayState — добавление
```

```cpp
GameMode m_gameMode = GameMode::Exploring;
```

Правила:
- `Exploring`: игрок может двигаться (WASD). После каждого MOVEMENT или TURN — перейти в `TurnWaiting`
- `TurnWaiting`: заблокировать ввод. Через 0.1s (или по кнопке действия) — перейти в `Exploring`
- В будущем здесь будет проверка: если монстр рядом → его ход

### Шаг 23: Блокировка ввода во время анимации

```
PlayState::HandleEvent / Update
```

Если `m_camera.IsAnimating()` или `m_gameMode != Exploring` — не обрабатывать WASD.
Добавить `m_queuedAction` — если игрок нажал клавишу во время анимации, выполнить после.

### Шаг 24: Индикатор режима (debug-ImGui)

В `PlayState::Render()`:
- Показывать текущий `GameMode`
- Показывать GridPosition и Direction
- Показывать "Press SPACE to end turn"

### Шаг 25: Тест пошаговости

Запустить, убедиться: после каждого шага (W/S/A/D) ввод блокируется на 0.1s.
Поворот — тоже шаг.

### Шаг 26: `TurnQueue` — очередь ходов

```
src/game/combat/turn_queue.h — новый модуль
```

```cpp
class TurnQueue final
{
public:
    void Advance(); // следующий ход
    [[nodiscard]] bool IsPlayerTurn() const noexcept;
    void SetPlayerTurn(bool b) noexcept;
private:
    int32_t m_currentActor = 0; // 0 = player
};
```

Пока один actor — игрок. Для будущего расширения.

### Шаг 27: Состояние `TurnAnimating` после фиксации

После `Advance()`: установить `m_gameMode = TurnAnimating` на время `ANIMATION_DURATION`.
По окончании анимации (в `Update()`) — проверить `m_camera.IsAnimating()`, если нет — вернуться к `Exploring`.

### Шаг 28: Подготовка к бою — `ActionType`

```
src/game/combat/action.h — новый файл
```

```cpp
enum class ActionType : uint8_t
{
    MoveForward,
    MoveBackward,
    TurnLeft,
    TurnRight,
    MeleeAttack,
    Wait
};
```

Пока используются только Move/Turn, остальные заготовлены.

---

## Этап 1.4: Персонаж и монстры (шаги 29–37)

### Шаг 29: `Character` — структура героя

```
src/game/combat/character.h — новый файл
```

```cpp
struct Character final
{
    std::string name = "Hero";
    int32_t level = 1;
    int32_t hp = 20;
    int32_t maxHp = 20;
    int32_t ac = 12;        // armor class
    int32_t str = 12;
    int32_t dex = 12;
    int32_t con = 12;
    int32_t atkBonus = 2;   // attack bonus
    int32_t damageMin = 1;
    int32_t damageMax = 6;
    GridPosition position;
    Direction facing = Direction::North;
};
```

### Шаг 30: `Monster` — структура монстра

```
src/game/combat/monster.h — новый файл
```

```cpp
enum class MonsterAI : uint8_t
{
    Stationary, // стоит на месте, атакует в упор
    Patrol,     // ходит туда-сюда
    Aggressive  // преследует игрока
};

struct Monster final
{
    std::string name = "Rat";
    int32_t level = 1;
    int32_t hp = 5;
    int32_t maxHp = 5;
    int32_t ac = 10;
    int32_t atkBonus = 0;
    int32_t damageMin = 1;
    int32_t damageMax = 3;
    GridPosition position;
    Direction facing = Direction::North;
    MonsterAI ai = MonsterAI::Stationary;
    bool alive = true;
};
```

### Шаг 31: `MonsterManager` — управление монстрами

```
src/game/combat/monster_manager.h/.cpp — новый модуль
```

```cpp
class MonsterManager final
{
public:
    void Spawn(Monster monster);
    void Despawn(GridPosition pos);
    [[nodiscard]] Monster* At(GridPosition pos);
    [[nodiscard]] const std::vector<Monster>& All() const noexcept;
    void UpdateAI(Dungeon& dungeon); // для патрульных
    void RemoveDead();
private:
    std::vector<Monster> m_monsters;
};
```

### Шаг 32: Размещение тестового монстра

В `PlayState::OnEnter()`:
```cpp
Monster rat;
rat.name = "Rat";
rat.position = {5, 5, 0};
rat.hp = 5;
m_monsterManager.Spawn(rat);
```

### Шаг 33: MonsterRenderer — биллборд для монстра

```
src/game/combat/monster_renderer.h/.cpp — новый модуль
```

Класс для рендера монстров:
- Один billboard quad (2 треугольника, текстурированный)
- Для каждого живого монстра — Submit с матрицей позиции (повёрнут к камере? Нет — всегда смотрит в свою сторону)
- Пока процедурная текстура-заглушка (яркий квадрат с крестом)

### Шаг 34: `Dice` — утилита для бросков

```
src/game/combat/dice.h — новый файл
```

```cpp
namespace Dice
{
    int32_t Roll(int32_t sides); // d20, d6 и т.д.
    int32_t Roll(int32_t count, int32_t sides); // 2d6
}
```

Использовать `std::mt19937` (thread_local singleton).

### Шаг 35: Система атаки — `CombatSystem`

```
src/game/combat/combat_system.h/.cpp — новый модуль
```

```cpp
struct AttackResult final
{
    bool hit = false;
    int32_t damage = 0;
    bool critical = false;
    bool killed = false;
};

class CombatSystem final
{
public:
    [[nodiscard]] AttackResult MeleeAttack(const Character& attacker, Monster& defender);
    [[nodiscard]] AttackResult MonsterMeleeAttack(const Monster& attacker, Character& defender);
};
```

Формула: `d20 + attackBonus >= targetAC` → hit. Damage: `damageMin..damageMax`.
Critical: `d20 == 20` → double damage.

### Шаг 36: Мясо атаки — проверка дистанции

Правило: атака возможна, если монстр в соседней клетке перед игроком (по направлению взгляда).
Функция `IsAdjacentForward(GridPosition player, Direction facing, GridPosition monster) → bool`.

### Шаг 37: Связка: игрок нажимает E → MeleeAttack

- Добавить биндинг `Action_Attack` → `SDL_SCANCODE_E`
- В `HandleEvent` или `Update`: если `IsActionPressed("Action_Attack")` и `m_gameMode == Exploring`:
  1. Найти монстра перед игроком
  2. Если есть — вызвать `CombatSystem::MeleeAttack`
  3. Вывести результат (пока в лог/ImGui)
  4. Если монстр жив — его ответная атака
  5. Перейти в `TurnWaiting`

---

## Этап 1.5: Боевой интерфейс и логирование (шаги 38–44)

### Шаг 38: `CombatLog` — массив строк

```
src/game/ui/combat_log.h — новый файл
```

```cpp
class CombatLog final
{
public:
    void Add(std::string_view msg);
    void Clear();
    [[nodiscard]] const std::vector<std::string>& Messages() const noexcept;
    static constexpr size_t MAX_MESSAGES = 128;
private:
    std::vector<std::string> m_messages;
};
```

### Шаг 39: Вывод CombatLog в ImGui

В `PlayState::Render()`:
- ImGui-окно "Log" внизу экрана
- Вывод последних 10 сообщений
- Автоскролл

### Шаг 40: Раскраска сообщений

- "Hero hits Rat for 4 damage!" — белый
- "Rat hits Hero for 2 damage!" — жёлтый
- "Critical hit!" — красный
- "Rat dies!" — зелёный

Хранить `struct LogEntry { std::string text; glm::vec3 color; }`.

### Шаг 41: HUD — HP игрока

В `PlayState::Render()`:
- ImGui-окно "Hero" (слева или углу)
- HP bar: `hp / maxHp` с цветом (зелёный > 50%, жёлтый > 25%, красный)
- Level, AC

### Шаг 42: Монстр под курсором — подсказка

Если перед игроком есть монстр — показывать его имя и HP в UI.
Функция `GetMonsterInFront() → Monster*`.

### Шаг 43: Death handling

Когда `hp <= 0`:
- `m_gameMode = GameOver`
- ImGui-окно "Game Over" с кнопкой "Restart"
- `Restart` → `m_machine.ReplaceState("Play")` (пересоздаёт состояние)

### Шаг 44: Тестовый проход

Проверить:
- Герой ходит по комнате 8×8
- Монстр виден как биллборд
- Подошёл лицом к монстру, нажал E — атака, сообщения в логе
- Монстр атакует в ответ
- Монстр умирает, исчезает

---

## Этап 1.6: Предметы и инвентарь (шаги 45–52)

### Шаг 45: `Item` и `ItemType`

```
src/game/combat/item.h — новый файл
```

```cpp
enum class ItemType : uint8_t
{
    Weapon, Armor, Shield, PotionHeal,
    PotionMana, Key, Gold, Scroll, QuestItem
};

struct Item final
{
    std::string name;
    ItemType type = ItemType::Gold;
    int32_t value = 0;    // heal amount, gold value, etc.
    int32_t bonus = 0;    // atk/ac bonus
};
```

### Шаг 46: `Inventory` — контейнер предметов

```
src/game/combat/inventory.h — новый файл
```

```cpp
class Inventory final
{
public:
    bool Add(const Item& item);
    bool Remove(size_t index);
    void Clear();
    [[nodiscard]] const Item* Get(size_t index) const noexcept;
    [[nodiscard]] size_t Size() const noexcept;
    static constexpr size_t MAX_ITEMS = 32;
private:
    std::vector<Item> m_items;
};
```

### Шаг 47: Интеграция инвентаря в Character

Добавить `Inventory inventory` в `Character`.
Добавить оборудование: `std::array<const Item*, 3> equipment{};` (weapon, armor, shield).

### Шаг 48: Предметы на полу

`ItemDrop` — структура на клетке подземелья.
```cpp
struct ItemDrop final
{
    Item item;
    GridPosition position;
};
```

Добавить `m_itemDrops : std::vector<ItemDrop>` в `PlayState`.

### Шаг 49: Подбирание предметов (G)

- Кнопка `G` (Action_Pickup)
- Проверить, есть ли ItemDrop на клетке игрока
- Если да — `inventory.Add(item)`, удалить дроп
- Сообщение в лог

### Шаг 50: Рендер предметов на полу

Биллборд-спрайт для каждого ItemDrop (как монстры, но другой цвет/форма).
Пока — цветной квадратик.

### Шаг 51: Инвентарь UI (ImGui)

ImGui-окно "Inventory" (клавиша I):
- Список предметов
- Кнопка "Use" для PotionHeal (восстанавливает HP)
- Кнопка "Drop" (создаёт ItemDrop на полу)

### Шаг 52: Тест предметов

- Разместить тестовый ItemDrop (PotionHeal) на старте
- Подойти, нажать G — предмет в инвентаре
- Открыть инвентарь (I), использовать зелье — HP восстанавливается

---

## Этап 1.7: Сохранение/загрузка прототипа (шаги 53–56)

### Шаг 53: Сериализация PlayerCharacter

JSON через nlohmann:
```cpp
void to_json(json& j, const Character& c);
void from_json(const json& j, Character& c);
```

Поля: name, level, hp, maxHp, ac, str, dex, con, position (row, col, floor), facing, inventory.

### Шаг 54: Сериализация Dungeon (состояние)

```cpp
void to_json(json& j, const LevelData& level);
void from_json(const json& j, LevelData& level);
```

Для каждого Tile: тип, триггеры.
Для ItemDrop: предмет + позиция.
Для Monster: имя, hp, позиция, ai.

### Шаг 55: Save/Load в PlayState

- Кнопка F5 — `SaveGame("save.json")`
- Кнопка F9 — `LoadGame("save.json")`
- `SaveGame`: собрать json из Character, Dungeon, Monsters, ItemDrops
- `LoadGame`: распарсить, восстановить состояние
- Использовать `fopen`/`fwrite`/`fread` (WASM-safe)

### Шаг 56: Тест сохранения

- Поиграть, поднять предмет, убить монстра
- Сохранить (F5)
- Перезапустить игру, загрузить (F9)
- Всё на месте

---

## Этап 1.8: Полировка прототипа (шаги 57–60)

### Шаг 57: Ограничение видимости (FOV)

```cpp
bool IsVisible(GridPosition from, Direction facing, GridPosition target) const noexcept;
```

Классический roguelike FOV: только клетки перед игроком, ~5 клеток вперёд + 1 влево/вправо.
Игрок видит только стены перед собой. Всё, что за стеной — не рендерить.

Реализация: перед рендером DungeonRenderer помечать видимые клетки.

### Шаг 58: Туман войны

Невидимые клетки — чёрные (не рендерить).
Ранее виденные — тёмно-серые (dimmed).
Флаг `Tile::explored` и `Tile::visible`.

### Шаг 59: Карта (Tab)

ImGui-окно "Map" (Tab):
- Вид сверху на текущий этаж
- Игрок — зелёная точка
- Монстры — красные точки
- Исследованные клетки — серые
- Неисследованные — чёрные

### Шаг 60: Тест стабильности

- 15 минут геймплея без вылетов
- Все переходы между режимами
- Save/Load работает
- Монстры не появляются в стенах
- UI не ломается

---

# Фаза 2: Полноценный dungeon crawler (шаги 61–120)

> Цель: Eye of Beholder-style party-based RPG с отрядом до 6 героев,
> формацией, продвинутой боевой системой, заклинаниями, несколькими этажами.

## Этап 2.1: От одного героя к партии (шаги 61–68)

### Шаг 61: `Party` — класс отряда

```
src/game/combat/party.h — новый модуль
```

```cpp
class Party final
{
public:
    Character& operator[](size_t i);
    [[nodiscard]] const Character& operator[](size_t i) const;
    [[nodiscard]] size_t Size() const noexcept;
    void AddCharacter(const Character& c);
    [[nodiscard]] int32_t PartyLevel() const noexcept; // средний уровень
    [[nodiscard]] bool AllDead() const noexcept;
    static constexpr size_t MAX_SIZE = 6;
private:
    std::vector<Character> m_members;
};
```

### Шаг 62: Формация (строй)

```cpp
enum class Formation : uint8_t
{
    Line,     // в ряд (до 6)
    DoubleLine, // 2 ряда по 3
    Single     // колонна по одному
};

struct PartyPosition final
{
    int32_t memberIndex;
    GridPosition worldPos; // вычисляется из позиции партии + формации
};
```

Функция `ComputePartyPositions(GridPosition partyPos, Direction facing, Formation f) → std::vector<GridPosition>`

### Шаг 63: Рендер партии

Каждый член партии — биллборд или простая фигура (пока разноцветный квадрат).
Позиция вычисляется из формации.
Первый в строю — передний ряд, tank; второй — поддержка и т.д.

### Шаг 64: Смена переднего ряда

Если передний персонаж умирает — следующий занимает его место.
При нажатии `Q/E` (с шифтом) — перестановка членов партии в строю.

### Шаг 65: Экран создания персонажей

ImGui-окно "Create Party":
- Выбор имени, класса (Warrior, Cleric, Mage, Thief)
- Распределение характеристик (str, dex, con, int, wis, cha) — roll или manual
- Добавление до 4 персонажей
- Кнопка "Start Game"

### Шаг 66: Сериализация партии

JSON: массив Character → Party.

### Шаг 67: Партийный инвентарь

Общий инвентарь партии (несколько страниц).
Персонажи экипируют предметы из общего инвентаря.

### Шаг 68: Распределение опыта

При убийстве монстра — опыт делится между живыми членами партии.
Уровень растёт каждые N опыта.

---

## Этап 2.2: Продвинутая боевая система (шаги 69–77)

### Шаг 69: Инициатива

Каждый раунд: каждый участник (игрок + монстры) бросает `d20 + dexMod`.
Очередь ходов сортируется по убыванию.

### Шаг 70: Множество действий

```cpp
enum class CombatAction : uint8_t
{
    Move,        // 1 клетка
    Turn,        // 90°
    Attack,      // melee
    CastSpell,   // заклинание
    UseItem,     // использовать предмет
    Defend,      // +2 AC до следующего хода
    Wait         // пропуск
};
```

### Шаг 71: Action Points (AP)

Каждый ход: 2 AP.
- Move: 1 AP
- Turn: 0 AP (бесплатно, 1 раз за ход)
- Attack: 1 AP
- CastSpell: 2 AP
- UseItem: 1 AP
- Defend: 2 AP

### Шаг 72: Target selection

При атаке/заклинании: выбор цели.
Если монстр один перед партией — авто-цель.
Если несколько — цикличный выбор (Tab) или UI-список.

### Шаг 73: Monster AI — расширение

- `Stationary`: стоит, атакует первого в радиусе 1
- `Patrol`: ходит по заданному маршруту
- `Aggressive`: преследует игрока до 8 клеток, затем возвращается
- `Boss`: специальные атаки, вызывают подмогу

### Шаг 74: Monster groups

Монстры спавнятся группами (2–6).
Опыт за группу делится.

### Шаг 75: Боевой UI

ImGui-окно "Combat":
- Список действий (Move, Attack, Spell, Item, Wait)
- Отображается только в бою
- При выборе CastSpell — открывается список заклинаний

### Шаг 76: Анимация атаки

- Камера дёргается вперёд на 0.1s при ударе
- Монстр мигает красным при попадании
- При смерти монстра — fade out за 0.3s

### Шаг 77: Боевой лог — расширение

Цветовые категории:
- Белый: информация
- Жёлтый: попадание по монстру
- Красный: попадание по партии
- Зелёный: убийство / лечение
- Синий: заклинания
- Фиолетовый: критические эффекты

---

## Этап 2.3: Заклинания и магия (шаги 78–85)

### Шаг 78: Spell struct

```cpp
enum class SpellType : uint8_t
{
    Damage, Heal, Buff, Debuff, Light, Teleport, Shield
};

struct Spell final
{
    std::string name;
    SpellType type;
    int32_t minLevel;      // минимальный уровень персонажа
    int32_t manaCost;
    int32_t minDamage;
    int32_t maxDamage;
    int32_t duration;      // 0 = мгновенное
    std::string description;
};
```

### Шаг 79: Spellbook

У каждого персонажа-мага: `std::vector<Spell>`.
На старте: 2-3 базовых заклинания (Magic Missile, Light, Heal).

### Шаг 80: Мана (MP)

```cpp
int32_t mp = 10;
int32_t maxMp = 10;
```

Добавить в Character. Восстанавливается при отдыхе (R — rest, 1 раз на этаж).

### Шаг 81: Реализация заклинаний

- Magic Missile: `2d4` урона, дальняя атака (до 4 клеток)
- Light: освещает текущую и соседние клетки на 50 ходов
- Heal: `1d8 + wisMod` HP
- Shield: +2 AC на 3 хода

### Шаг 82: UI заклинаний

При выборе "Cast Spell" в бою:
- Список доступных заклинаний (с cost mana)
- Grey-out если не хватает маны
- Выбор цели для damage/heal

### Шаг 83: Магические монстры

Монстры-маги (Goblin Shaman): могут кастовать заклинания.
`Monster::spells : std::vector<Spell>`.

### Шаг 84: Свитки

ItemType::SpellScroll — одноразовое заклинание.
Использование из инвентаря = каст без траты маны, свиток исчезает.

### Шаг 85: Магические предметы

- Staff of Fire: +1d4 fire damage на атаку
- Ring of Protection: +1 AC
- Amulet of Healing: +2 при лечении

---

## Этап 2.4: Подземелье — несколько этажей (шаги 86–93)

### Шаг 86: Лестницы

```cpp
TileType::StairsDown
TileType::StairsUp
```

При наступлении на StairsDown — переход на следующий этаж.
Dungeon::GoToFloor(int32_t floor).

### Шаг 87: Генерация нескольких этажей

Пока вручную:
- Floor 1: 8×8, комната + коридор, 2 монстра
- Floor 2: 10×10, две комнаты, 3 монстра, лестница на 1
- Floor 3: 12×12, три комнаты, ловушки, босс

### Шаг 88: Прокрученные блокировки — ключи и двери

```cpp
TileType::DoorClosed
```

Требуется `ItemType::Key` с соответствующим `item.keyId`.
При нажатии `E` у закрытой двери: если есть ключ — дверь открывается (TileType::DoorOpen).
Если нет — сообщение "Door is locked".

### Шаг 89: Секретные стены

```cpp
TileType::SecretWall
```

Стена, которая выглядит как обычная, но проходима.
Обнаруживается при поиске (Search action, кнопка `F`).
Шанс обнаружения: `d20 + wisMod >= 15`.

### Шаг 90: Ловушки

```cpp
struct Trap final
{
    GridPosition position;
    int32_t damageMin = 5;
    int32_t damageMax = 10;
    bool triggered = false;
    bool detected = false;
};
```

При наступлении на необнаруженную ловушку: урон по переднему персонажу.
Обнаружение: Search (как секретные стены).

### Шаг 91: Рендер лестниц и дверей

- StairsDown: тёмный квадрат в центре клетки
- StairsUp: светлый квадрат
- DoorClosed: коричневый прямоугольник
- DoorOpen: зелёная пунктирная линия

### Шаг 92: Мини-карта с этажами

В окне карты (Tab) добавить:
- Индикатор текущего этажа
- Лестницы отмечены
- Лифты/телепорты

### Шаг 93: Сброс состояния при смене этажа

Сохранять состояние предыдущего этажа (монстры, двери, предметы).
При возврате — восстанавливать.

---

## Этап 2.5: Ретро UI (шаги 94–103)

### Шаг 94: BitmapFont — текстурный шрифт

```
src/game/ui/bitmap_font.h/.cpp — новый модуль
```

- Загрузить текстуру атласа символов (16×16 глифов)
- `RenderText(const std::string& text, int x, int y, float scale)`
- Использовать отдельный ортографический шейдер + quad на символ

### Шаг 95: Ортографический проход рендера

```
GameApp::render() — расширение
```

После FBO blit но до ImGui:
1. Bind ортографический шейдер
2. Нарисовать HUD (прямоугольники, текст)
3. Unbind

### Шаг 96: HUD — портреты партии

Слева: портреты членов партии (пока цветные квадраты 48×64).
Рядом: HP bar, AC, статус (Poisoned, Silenced и т.д.).
Активный персонаж подсвечен.

### Шаг 97: HUD — лог сообщений

Внизу экрана: 4 строки текста через BitmapFont.
Автоскролл, не-ImGui (чтобы был всегда).

### Шаг 98: HUD — мини-карта

Правый верхний угол: 5×5 клеток вокруг игрока.
Пиксельная мини-карта через ортографический quad.

### Шаг 99: HUD — индикатор направления

Компас: N/S/E/W, текущее направление подсвечено.

### Шаг 100: Отключение ImGui для game UI

ImGui остаётся только для debug-панелей (Profiler, Camera debug).
HUD рисуется через ортографический проход.
ImGui-меню остаётся только для MainMenu/Settings.

### Шаг 101: Анимации переключения между UI-режимами

Fade in/out при открытии инвентаря, карты, меню.

### Шаг 102: Пиксельные текстуры

Замена процедурных checkerboard на рукописные 16×16/32×32 текстуры:
- stone_wall.png — стена
- floor_stone.png — пол
- hero_portrait.png — портрет
- rat_monster.png — спрайт крысы
- ui_panel.png — рамка HUD

### Шаг 103: Цветовая палитра EGA

Ограничить палитру до 16/64 цветов (EGA/VGA retro).
Каждая текстура — indexed color.

---

## Этап 2.6: Звук и музыка (шаги 104–108)

### Шаг 104: Ambient — эхо подземелья

Циклический OGG (зацикленный эмбиент).
Загрузить через `AudioSystem::Load()`, играть на канале `Music`.
Громкость из настроек.

### Шаг 105: Звуковые эффекты

WAV:
- step.wav — шаг по клетке
- door_open.wav — открытие двери
- sword_swing.wav — взмах
- hit.wav — попадание
- monster_die.wav — смерть монстра
- spell.wav — каст заклинания
- pickup.wav — подбор предмета
- trap.wav — ловушка

### Шаг 106: Привязка звуков к событиям

Через `EventBus`:
- `StepEvent` → Play(step.wav)
- `AttackEvent` → Play(sword_swing.wav)
- `HitEvent` → Play(hit.wav)
- `MonsterDeathEvent` → Play(monster_die.wav)
- `PickupEvent` → Play(pickup.wav)
- `SpellCastEvent` → Play(spell.wav)
- `TrapTriggerEvent` → Play(trap.wav)

### Шаг 107: AudioSystem — тонкая настройка

- Volume sliders (Music/SFX) из Settings
- Mute кнопка
- Проверка: не играть звук, если такой же уже играет (для шагов)

### Шаг 108: MIDI-подобный фон (опционально)

Процедурная музыка через SDL_AudioStream (простые тоны, квадратные волны).
Ретро-стиль: как NES/FM-синтез.

---

## Этап 2.7: Полировка и контент (шаги 109–120)

### Шаг 109: Dungeon — процедурная генерация

Алгоритм BSP (Binary Space Partition):
- Разделить область на комнаты
- Соединить коридорами
- Разместить монстров, предметы, лестницы
- На каждом этаже — больше комнат, сильнее монстры

### Шаг 110: Random encounter на лестницах

10% шанс при переходе на этаж: монстры на стартовой комнате.
Сообщение "Something stirs in the darkness..."

### Шаг 111: Торговец

Особая клетка (TileType::Merchant):
- Открыть окно покупки/продажи
- Торговец продаёт случайные предметы
- Цены: покупка ×2, продажа ×0.5

### Шаг 112: Квестовые предметы

ItemType::QuestItem — не продаются, не выбрасываются.
Связаны с событием (например, "Find the Lost Crown of Kings").

### Шаг 113: Триггеры и события

```cpp
struct Trigger final
{
    int32_t id;
    GridPosition position;
    TriggerAction action; // enum: Teleport, Dialog, SpawnMonsters, ChangeTile
    int32_t actionData;   // floor/room id для телепорта, text id для диалога
};
```

### Шаг 114: Система диалогов

Простые текстовые окна (как в EoB):
- При наступлении на триггер
- При разговоре с NPC
- Пошаговый текст с кнопкой Continue (Space)

### Шаг 115: Difficulty scaling

С каждым этажом:
- +1 к AC/atk/damage монстров
- Больше монстров в комнатах
- Лучшие предметы в сундуках

### Шаг 116: Stats экран (C)

Окно персонажа:
- Все характеристики
- Экипировка
- Resistances
- Experience / next level

### Шаг 117: Rest (R)

Отдых на месте:
- Восстанавливает HP/MP (20% от макс)
- Требует 10 ходов
- 20% шанс случайной встречи во время отдыха
- Нельзя отдыхать, если монстры рядом (3 клетки)

### Шаг 118: Сохранение прогресса

Auto-save при смене этажа.
Quick save / quick load как в прототипе (F5/F9).
Слоты сохранения (3 слота).

### Шаг 119: Балансировка

- Тест: прохождение Floor 1–3 без смерти
- Настройка HP монстров, урона, количества предметов
- Убедиться, что мана не кончается слишком быстро
- Кривая сложности: плавный рост

### Шаг 120: Сборка и релиз прототипа

- `build.bat` — финальная проверка
- MSVC сборка — финальная проверка
- README.MD — инструкция
- CHANGELOG.md
- EXE + DLL в bin/

---

## Технический долг (cross-cutting)

### Каждый спринт

- [Build] Проверять сборку под g++ и MSVC
- [Test] Добавлять unit-тесты для новой логики
- [Engine] Профилировать через Profiler, не допускать падения FPS ниже 60
- [Code] Проверять на соответствие AGENTS.md

### После Фазы 1

- [Engine] Emscripten-сборка для Web
- [Engine] Asset pipeline: загрузка .png/.ogg из папки assets/
- [Test] Интеграционные тесты для combat/dungeon

### После Фазы 2

- [Content] Полный набор ретро-текстур (16×16, EGA-палитра)
- [Content] 5+ этажей подземелья
- [Content] 10+ типов монстров
- [Content] 15+ заклинаний
- [Content] 30+ предметов
- [Engine] Оптимизация: portalling, PVS, instancing стен
- [Engine] Система сохранений с облачным слотом (опционально)
