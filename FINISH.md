# Реализовано (Phase 1: steps 1–44)

> Всё завершённое из ROADMAP.md, перемещено сюда.
> Актуальный план оставшихся задач — в [ROADMAP.md](./ROADMAP.md).

---

## Этап 1.1: Переход от FPS-камеры к грид-системе (шаги 1–10) ✅

### Шаг 1: `GridPosition`
`src/game/grid_position.h` — POD-структура с `row, col, floor`, `operator==`, `std::hash`.

### Шаг 2: `Direction`
`src/game/direction.h` — `North/East/South/West` с функциями `DirectionFromIndex`, `NextDirection`, `DirectionToVec`, `DirectionToYaw`.

### Шаг 3: Расширение `Camera` в GridCamera
`src/engine/renderer/camera.h/.cpp` — добавлены `m_gridPosition`, `m_gridFacing`, `m_targetPosition`, `m_targetYaw`, `m_isAnimating`, `m_animationTimer`. Методы `SetGridPosition`, `SnapToGrid`, `IsAnimating`, `TurnLeft/Right`, `MoveForward/Backward/Left/Right`.

### Шаг 4: Lerp-анимация
`Camera::UpdateAnimation(float dt)` — lerp позиции и yaw с учётом 360° wrapping. `ANIMATION_DURATION = 0.2f`, `GRID_UNIT = 1.0f`.

### Шаг 5–6: Вращение 90° и движение на 1 клетку
`TurnLeft/TurnRight` — поворот на месте. `MoveForward/Backward/Left/Right` — шаг на 1 клетку. Strafe (Q/E) добавлен сверх плана.

### Шаг 7–8: Отключение FPS-управления
WASD → grid-управление. Mouse-look только в debug-режиме (Tab). Кубы удалены из `OnEnter()`.

### Шаг 9: `Cell` и `Chunk`
`src/game/dungeon/chunk.h` — `Cell { hasFloor, hasCeiling, isWall }`, `Chunk` — `std::array<Cell, 1225>` (35×35) на стеке.

**Отклонение**: вместо `enum class TileType / LevelData` — структура с флагами. `GenerateTestRoom()` — комнаты + коридоры + пилоны.

### Шаг 10: Интеграция `Dungeon` в `PlayState`
`Dungeon m_dungeon` как член `PlayState`, `GenerateTestRoom()` в `OnEnter()`, проверка `m_dungeon.IsWalkable(targetPos)` перед движением.

---

## Этап 1.2: Рендер подземелья (шаги 11–20) ✅

### Шаг 11–12: Генератор геометрии пола и стен
`DungeonGeometry` — пол (квады y=0, нормаль 0,1,0), стены (4 грани на клетку стены, нормали наружу), потолок (квады y=1, нормаль 0,-1,0). Winding CCW, проверены.

### Шаг 13: `DungeonRenderer`
`src/game/dungeon/dungeon_renderer.h/.cpp` — управление геометрией dungeon, `Build()`/`Submit()`.

### Шаг 14: Шейдеры dungeon
`DUNGEON_VERT_SRC` / `DUNGEON_FRAG_SRC` — UBO ViewProjection, текстура × ambient (0.3) + directional light (0.7). `discard` для прозрачных пикселей.

### Шаг 15–16: Текстуры и материалы
Из файлов: `data/texture_08.png` (пол), `texture_10.png` (стена), `texture_11.png` (потолок). Материалы: `mat_dungeon_floor/wall/ceiling`.

### Шаг 17: Подключение DungeonRenderer в PlayState
`RenderScene()` — `m_dungeonRenderer.Submit(renderer)`.

### Шаг 18: FBO с ретро-разрешением
FBO с `GL_NEAREST`, настраиваемое разрешение.

### Шаг 19–20: Позиционирование камеры, визуальная проверка
Старт: `{17, 17, 0}` North. Камера y=0.5. Анимация рывков, поворот 90° с wrapping.

---

## Этап 1.3: Пошаговый режим и ввод (шаги 21–28) ✅

### Шаг 21: `GameMode`
`Exploring`, `TurnWaiting`, `TurnAnimating`, `CombatTurn`, `GameOver`.

### Шаг 22: Конечный автомат ходов
`m_gameMode` управляет: Exploring (движение + auto-repeat), после движения → TurnWaiting → (Advance очереди) → TurnAnimating / Exploring. Поворот (A/D) не потребляет ход.

### Шаг 23: Блокировка ввода
TurnWaiting — мгновенно (0 задержки). Edge-triggered разрешён в Exploring и TurnAnimating. Held-repeat — только в Exploring.

### Шаг 24: Индикатор режима (debug-ImGui)
Текущий GameMode, номер актора, GridPosition, Direction, Move Repeat Delay.

### Шаг 25: Поворот — не шаг
Только движение W/S/Q/E потребляет ход. A/D — бесплатно.

### Шаг 26: `TurnQueue`
`src/game/combat/turn_queue.h` — очередь ходов. Пока 1 actor (игрок). `Advance()` при count=1 всегда возвращает на игрока.

### Шаг 27: TurnAnimating
После Advance: если камера анимируется → `TurnAnimating`. Edge-triggered пробивает анимацию.

### Шаг 28: `ActionType`
`src/game/combat/action.h` — `MoveForward/Backward, TurnLeft/Right, StrafeLeft/Right, MeleeAttack, Wait`.

---

## Этап 1.4: Персонаж и монстры (шаги 29–37) ✅

### Шаг 29: `Character`
`src/game/combat/character.h` — `name, level, hp, maxHp, ac, str, dex, con, atkBonus, damageMin/Max, position, facing`.

### Шаг 30: `Monster` и `MonsterAI`
`src/game/combat/monster.h` — `Stationary/Patrol/Aggressive`, `Monster` с полями как у Character + `alive, ai`.

### Шаг 31: `MonsterManager`
`src/game/combat/monster_manager.h/.cpp` — `Spawn`, `Despawn`, `At` (поиск по позиции), `RemoveDead`, `All`, `UpdateAI` (пока пусто).

### Шаг 32: Тестовые монстры в `OnEnter()`
Скелет (`{16,17}`, South, HP 8, AC 12) и слизь (`{14,17}`, South, HP 4, AC 8).

### Шаг 33: `MonsterRenderer` — биллборд
`src/game/combat/monster_renderer.h/.cpp` — Y-up billboard (поворот вокруг Y к камере). Текстуры `skeleton.png` / `slime.png` из `bin/data/`.

### Шаг 34: `Dice`
`src/game/combat/dice.h/.cpp` — `Dice::Roll(sides)` / `Roll(count, sides)` через `std::mt19937`.

### Шаг 35–36: `CombatSystem`
`src/game/combat/combat_system.h/.cpp` — `MeleeAttack` (Character→Monster) и `MonsterMeleeAttack` (Monster→Character). Формула: `d20 + atkBonus >= AC`. Critical: d20==20 → double damage. Атака только в клетку перед игроком.

### Шаг 37: Space → MeleeAttack
`Action_Attack` → `SDL_SCANCODE_SPACE`. Edge-triggered. Находит монстра впереди, атакует, при выживании — counter-attack, затем `TurnWaiting`.

---

## Этап 1.5: Боевой интерфейс и логирование (шаги 38–44) ✅

### Шаг 38: `CombatLog`
`src/game/ui/combat_log.h/.cpp` — `LogEntry{text, color}`, `Add()`, `Clear()`, MAX_ENTRIES=128.

### Шаг 39–40: Вывод в ImGui с раскраской
Правая панель (300px), автоскролл, кнопка Clear. Цвета: белый (hit), жёлтый (damage), красный (critical), зелёный (killed), серый (miss).

### Шаг 41: Hero HUD
Левая панель: HP bar (ProgressBar), AC, Attack Bonus, Damage dice.

### Шаг 42: Монстр спереди
Окно "Monster Ahead" с именем, HP bar, AC. Показывается только если монстр в клетке перед игроком.

### Шаг 43: Death handling
`hp ≤ 0` → `GameOver`. ImGui-popup по центру, блокировка ввода (кроме Tab debug), кнопка "Back to Menu".

### Шаг 44: Визуальная проверка
- Герой ходит по комнате
- Монстры видны (биллборды, текстуры с прозрачностью)
- Подошёл лицом → Space → атака, лог, counter-attack
- Монстры блокируют проход (проверка `MonsterManager::At` в `isWalkableAction`)
- Смерть → GameOver

---

## Ключевые отклонения от оригинального плана

- **TileType → Cell** (hasFloor/hasCeiling/isWall, без enum)
- **LevelData → Chunk** (фиксированный 35×35, массив на стеке)
- **Текстуры** из файлов, не процедурные
- **Управление**: W/S — вперёд/назад, A/D — поворот, Q/E — стрейф
- **TurnWaiting** без задержки 0.1s — обрабатывается мгновенно
- **Поворот не потребляет ход** — A/D не триггерит TurnWaiting
- **Attack → Space** (не E, т.к. E занято стрейфом)
- **Monster Y-billboard** — вершины `y=[-0.5..0.5]`, центрировано относительно `GridToWorld(y=0.5)`
- **Monster collision** — проверка `MonsterManager::At` в `isWalkableAction`
- **Прозрачность** — `discard` во фрагментном шейдере

## Дополнительно (не в изначальном roadmap)

- Система чанков 35×35
- Strafe (MoveLeft/MoveRight) — Q/E
- Auto-repeat с настраиваемой задержкой
- Множественные зажатые клавиши (edge-trigger прерывает анимацию)
- Debug-камера (Tab) с сохранением/восстановлением grid-позиции
- Исправление yaw-wrapping в Camera::UpdateAnimation
- Иммутабельный `const Monster* At(pos) const` для константных проверок
- ImGui-окна с фиксированными стартовыми позициями и размерами
