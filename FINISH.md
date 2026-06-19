# Реализовано (Phase 1–4: steps 1–90)

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

---

## Этап 1.6: Предметы и инвентарь (шаги 45–52) ✅

### Шаг 45: `Item` и `ItemType`
`src/game/combat/item.h` — `ItemType` (Weapon, Armor, Shield, PotionHeal, PotionMana, Key, Gold, Scroll, QuestItem), `ItemRarity` (Common…Legendary), `Item { name, type, rarity, value, bonus }`.

### Шаг 46: `Inventory`
`src/game/combat/inventory.h/.cpp` — `Add`, `Remove`, `Clear`, `Get`, `Size`, MAX_ITEMS=32.

### Шаг 47: Интеграция инвентаря в Character
`Character::inventory` — член `Inventory`.

### Шаг 48: `ItemDrop` на полу
`ItemDrop { Item item; GridPosition position }` — структура в `item.h`. `PlayState::m_itemDrops` — вектор дропов на полу.

### Шаг 49: Подбирание предметов (Space + контекст)
- Space контекстно-зависим: если монстр впереди → атака, иначе если есть предмет под ногами → подобрать.
- `processPickup()` — поиск дропа на клетке игрока, добавление в инвентарь, сообщение в лог.

### Шаг 50: `ItemRenderer` — рендер предметов на полу
`src/game/combat/item_renderer.h/.cpp` — Y-биллборд (0.6×0.6), материал по типу предмета. Текстуры загружаются из `data/item_*.png` (5 скачаны из opengameart, остальные — цветные шахматные поля как fallback).

Скачанные спрайты:
- `item_potion_heal.png` — красная склянка
- `item_key.png` — золотой ключ
- `item_gold.png` — монета
- `item_scroll.png` — свиток
- `item_sword.png` — меч

### Шаг 51: Инвентарь UI (ImGui, клавиша I)
- `m_showInventory` — toggle по `I`.
- `renderInventoryWindow()` — список предметов, кнопки "Use" (зелья/свитки) и "Drop" (создаёт ItemDrop на текущей клетке).

### Шаг 52: Тест предметов
- На старте спавнятся: Healing Potion (`{16,17}`) и Gold Coins (`{14,17}`).
- Подойти → Space → предмет подобран, сообщение в логе.
- `I` → инвентарь → Use (восстанавливает HP) / Drop (кладёт на пол).

---

---

## Этап 1.7: Сохранение/загрузка прототипа (шаги 53–56) ✅

### Шаг 53: `serialization.h` — JSON сериализация Character

`src/game/serialization.h` — заголовочный файл с `to_json`/`from_json` для всех типов:
- `GridPosition`, `Direction`, `ItemType`, `ItemRarity`, `Item`, `ItemDrop`
- `MonsterAI`, `MonsterType`, `Monster`, `Cell`, `Chunk`, `Character`
- Character включает сериализацию инвентаря (перебор через `Get()`/`Size()`)
- Все функции `inline` для header-only подхода

### Шаг 54: Сериализация Dungeon в JSON

- `Cell`: три булевых поля (`hasFloor`, `hasCeiling`, `isWall`) сохраняются как JSON-объект
- `Chunk`: массив 35×35 = 1225 клеток с координатами `row`/`col`
- `Monster`: все поля сериализуются, включая тип, позицию, направление, AI
- `ItemDrop`: предмет + позиция
- В `Dungeon` добавлены `SetChunk()` и non-const `GetChunk()` для загрузки

### Шаг 55: Save/Load в PlayState

- **F5** — `SaveGame("save.json")`: собирает JSON (Character, Dungeon, Monsters, ItemDrops), `j.dump(2)` → `fopen/fwrite/fclose`
- **F9** — `LoadGame("save.json")`: `fopen/fread` → `json::parse` → восстановление состояния
- После загрузки:
  - Камера синхронизируется с позицией персонажа
  - DungeonRenderer помечен на перестроение (`SetNeedsRebuild(true)`)
  - `MonsterManager::Clear()` + спавн загруженных монстров
  - Режим сбрасывается в `Exploring`
- WASM-safe: используется только `fopen`/`fread`/`fwrite`, без `std::filesystem`/`thread`
- В `MonsterManager` добавлен метод `Clear()`

### Шаг 56: Тест сохранения

- Сборка пройдена (gcc, 0 ошибок)
- Запуск — игра стартует без ошибок
- F5 → файл `save.json` создаётся на диске
- F9 → состояние восстанавливается

---

## Этап 1.8: Полировка прототипа

### Шаг 57: Карта (Tab / M) ✅

`src/game/states/play_state.cpp` — `renderMapWindow()`:
- ImGui-окно **"Map"** (550×550, с масштабированием под dungeon 35×35)
- Каждая клетка заливается цветом:
  - Стена — тёмно-коричневый (`#3C3228`)
  - Пол — тёмно-серый (`#1E1E1E`)
  - Пустота — чёрный
- **Игрок** — зелёная точка с линией направления взгляда
- **Монстры** — красные точки с линией направления
- **Предметы** — маленькие синие точки
- Сетка (полупрозрачные линии для ориентирования)
- Показывается при:
  - **Tab** — вместе с debug-режимом
  - **M** — отдельно, независимо от debug

---

## Этап 1.8: Полировка прототипа

### Шаг 58: Death screen / Restart ✅

`PlayState::RestartGame()` — полный сброс состояния без перезапуска игры:
- Сброс `Character` (HP=20, позиция {17,17}, стартовые статы)
- Камера — SnapToGrid на стартовую клетку
- Dungeon — `GenerateTestRoom()` заново
- Монстры — очистка + респавн Skeleton и Slime
- Предметы — очистка + респавн Healing Potion и Gold
- CombatLog — очистка
- Game Over popup: кнопка **"Restart"** рядом с **"Back to Menu"**

### Шаг 59: Options window (F2) ✅

`PlayState::renderOptionsWindow()` — ImGui-окно "Options":
- **Move Repeat Delay** — слайдер (0.05–0.5 с), применяется мгновенно
- **Render Height** — слайдер (120–1080 px) для retro FBO (требует перезапуска)
- **Save** — запись в `player_config.json` (сохраняет остальные поля без изменений)
- **Cancel** — закрывает окно без сохранения
- Открывается по **F2** (не пересекается с F1 debug / Tab map)

### Шаг 60: Финальная сборка ✅

- **gcc (build.bat)**: чистая сборка, 0 ошибок, 0 warnings
- **MSVC (Game.slnx)**: проверено — требует Visual Studio с C++ tools (недоступен в текущей среде)
- **Smoke test**: игра запускается, все окна открываются, управление работает
- **Controls** в HUD обновлены: Tab (map), F1 (debug), F2 (options), F5/F9 (save/load), I (inventory)

---

### Исправления после тестирования
- **Размер предметов** — биллборд уменьшен с `0.6×0.6` до `0.3×0.3` (`±0.15` вместо `±0.3`)
- **Y-смещение предметов** — центр поднят с `y=0.1` до `y=0.15`, низ спрайта стоит на полу
- **Pickup с соседней клетки** — `processPickup()` проверяет и свою клетку, и клетку перед собой (по направлению взгляда)
- **Tab — только карта** — больше не переключает debug-режим, не сохраняет/восстанавливает позицию камеры
- **Debug-режим** — перенесён на F1 (сохраняет/восстанавливает камеру при входе/выходе)
- **Синхронизация позиции персонажа** — `m_character.position` и `m_character.facing` синхронизируются с камерой в `doGridAction()` после каждого шага/поворота

---

## Фаза 3: Skill System & Class Integration (шаги 41–55) ✅

### Skill Trees / System (шаги 41–48)

41. `SkillTree` — структура в JSON для каждого класса: массив умений с требованиями по уровню (реализовано через `abilities.json` с полями `levelReq`, `classReq`, `passive`)
42. `Skill` — имя, описание, уровень открытия, эффект, стоимость MP/кулдаун (`struct Skill` в `skill_manager.h`, парсинг из JSON через `Skill::FromJson()`)
43. Список доступных умений на данном уровне показывается в окне Skills (клавиша K) — `renderSkillsWindow()` с группировкой на Active/Passive, цветовая индикация (белый = доступно, серый = неоткрыто, тёмный = выше уровня)
44. Passive skills: "+5 HP", "+1 ATK", "+2 AC" — применяются автоматически через `ApplyPassiveSkills()` (рекалькуляция базовых статов + сумма всех passives)
45. Active skills: "Power Strike (2x damage)", "Shield Bash (stun)" — добавляются на hotbar через `GrantSkillsForLevel()`
46. Умения загружаются в `Character::unlockedSkills` при level-up — `GrantSkillsForLevel()` вызывается в момент повышения уровня
47. Hotbar (1-9) заполняется из `actionSlots[]` — массив `ActionSlot {type, id, cooldownRemaining}`, рендерится `renderHotbar()` с фоном, нумерацией, кулдаун-оверлеем
48. Окно "Skills" (K): дерево умений, подсвечены доступные, серые — неоткрытые (реализовано как список с цветовой дифференциацией)

### Интеграция классов (шаги 49–55)

49. `ClassSelectionState` → после выбора создаёт Character с класс-специфичными статами (использует `CreateCharacter()` из `class_selection_state.cpp`, читает `classes.json`)
50. Каждый класс получает 1–2 стартовых активных умения (например, Barbarian: "Power Strike", Mage: "Magic Missile") — задаётся в `classes.json` `startingAbilities`
51. Passive class bonuses: Barbarian +2 damage, Paladin +2 AC, Thief +1 backstab (через `ClassDamageBonus()`, `ClassAtkBonus()`, `ClassAcBonus()` в `CombatSystem`)
52. `CombatSystem` использует `Character::charClass` для модификаторов атаки — статические методы `ClassDamageBonus()`, `ClassAtkBonus()`, `ClassAcBonus()`
53. Thief: атака со спины (monster не смотрит на игрока) → +2 damage, +2 atkBonus (параметр `behind` в `MeleeAttack()`, расчёт через `DirectionToTarget()`)
54. Регенерация: Paladin медленно восстанавливает HP (2/ход), Mage восстанавливает MP (1/ход) — через `regenHpPerTurn`/`regenMpPerTurn` в `classes.json`, применяется в `applyRegen()` на старте хода игрока
55. Тест: создание персонажа каждого класса, проверка стартовых статов и умений — все 5 классов работают через `ClassSelectionState`

### Ключевые изменения

- `data/abilities.json` — добавлены поля `passive`, `hpBonus`, `mpBonus`, `acBonus`, `atkBonus`, `damageBonus` для passive-скиллов
- `src/game/data/skill_manager.h/.cpp` — новый файл: `Skill` struct, `SkillManager` (6 статических методов: `GetSkill()`, `GetSkillsForClass()`, `GetSkillsForClassAtLevel()`, `GetPassiveSkillsForClassAtLevel()`, `GetSkillIdsForClassAtLevel()`, `GetNewSkillIdsForLevel()`)
- `src/game/combat/character.h` — добавлены `unlockedSkills` (vector), `actionSlots[9]` (array of `ActionSlot`), `learnedSpells` (vector)
- `src/game/combat/combat_system.h/.cpp` — добавлен `UseAbility()`, статические `ClassDamageBonus()`, `ClassAtkBonus()`, `ClassAcBonus()`; backstab +2/+2
- `src/game/data/experience_system.h/.cpp` — добавлены `GrantSkillsForLevel()`, `ApplyPassiveSkills()`
- `src/game/states/class_selection_state.cpp` — `CreateCharacter()` назначает стартовые abilities в `unlockedSkills` + `actionSlots`
- `src/game/states/play_state.h/.cpp` — `renderHotbar()` (9 слотов, кулдаун, клавиши 1-9), `renderSkillsWindow()` (K, активные/пассивные, тултипы), `processActionSlot()`, `useAbility()`, `applyRegen()`; level-up popup вызывает `GrantSkillsForLevel()` + `ApplyPassiveSkills()`
- `src/game/serialization.h` — Character сериализует `unlockedSkills`, `actionSlots[9]`, `learnedSpells`; добавлены `to_json`/`from_json` для `ActionSlot`
- `src/core/json_data_manager.h` — удалён дубликат `AllAbilities()`; `AllAbilities()`, `GetLevelTable()` — `noexcept`

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

---

## Фаза 2: Фундамент JSON (шаги 1–25) ✅

### JSON-данные (шаги 1–10)

1. `data/classes.json` — 5 классов: Barbarian, Paladin, War Priest, Thief, Mage (базовые статы, HP, MP, STR, DEX, CON, INT, ATK, AC, allowedArmor/Weapons, startingAbilities, regen)
2. `data/monsters.json` — 7 типов монстров: skeleton, slime, goblin, skeleton_warrior, giant_rat, fire_elemental, skeleton_mage (каждый с level, HP, AC, ATK, дамаг, XP, AI, резисты, дроп)
3. `data/items_base.json` — базовые предметы: зелья, ключи, золото, ингредиенты, материалы, еда, броня, оружие, аксессуары, свитки
4. `data/weapon_types.json` — 8 типов оружия: dagger, sword, axe, mace, spear, staff, bow, crossbow (damage, range, hands, stat requirements, tags)
5. `data/armor_types.json` — 8 типов брони/защиты: cloth, leather, chain, plate, shield, helmet, boots, gloves (AC, slot, strReq, tags)
6. `data/materials.json` — 10 материалов: cloth, leather, wood, bone, iron, steel, silver, mithril, adamant, dragonbone (tier, damageMult, acBonus, weight, valueMult, tags)
7. `data/prefixes.json` — 13 префиксов: crude, rusty, old, plain, fine, sturdy, sharp, vicious, blessed, flaming, freezing, shocking, venomous, holy (модификаторы damage/AC/ATK, elementDamage, valueMult)
8. `data/postfixes.json` — 10 постфиксов: none, of_power, of_protection, of_accuracy, of_fire, of_ice, of_vitality, of_vampire, of_strength, of_dexterity (тэги, модификаторы, elementDamage)
9. `data/spells.json` — 7 заклинаний: magic_missile, fireball, frost_bolt, lightning_bolt, heal, smite_undead, holy_smite (тип, элемент, дамаг/хил, радиус, MP, требования, статус-эффекты)
10. `data/abilities.json` — 10 способностей: power_strike, holy_smite, heal, smite_undead, backstab, shadow_step, magic_missile, fireball, shield_bash, fire_aura (тип, MP, кулдаун, дамаг/хил, дальность, требования)

### Загрузчик данных (шаги 11–20)

11. `src/core/json_data_manager.h/.cpp` — синглтон `JsonDataManager`, загружает 10 JSON-файлов из `data/` при старте игры (`LoadAll("data")`), предоставляет `GetMonsterData(id)`, `GetItemBaseData(id)`, `GetClassData(id)`, `GetSpellData(id)`, `GetAbilityData(id)`, `GetMaterial(id)`, `GetPrefix(id)`, `GetPostfix(id)`, `GetWeaponType(id)`, `GetArmorType(id)`, а также bulk-геттеры `AllMonsters()`, `AllClasses()` и т.д.
12. `src/game/data/monster_factory.h/.cpp` — `MonsterFactory::Create(typeId, level)` — создаёт `Monster` из JSON-шаблона с level-scaling (HP + level*2, atkBonus + level/2, xpReward + level*5)
13. `src/game/data/item_factory.h/.cpp` — `ItemFactory::CreateBase(itemId)`, `CreatePotion(subtype, value)`, `CreateGold(amount)`, `CreateScroll(spellId)` — создаёт `Item` из данных
14. `src/game/data/spell_manager.h/.cpp` — `SpellManager::GetSpell(id)`, `GetSpellsByClass(classId)` — доступ к spells.json
15. `src/game/data/ability_manager.h/.cpp` — `AbilityManager::GetAbility(id)`, `GetAbilitiesByClass(classId)` — доступ к abilities.json
16. `src/game/data/class_manager.h/.cpp` — `ClassManager::GetClass(id)`, `ApplyStartingStats(...)` — применяет классовые статы
17. `src/game/data/material_generator.h/.cpp` — `MaterialGenerator::AssembleName(prefix, material, base, postfix)` — склейка имени предмета
18. `src/game/data/item_stat_calculator.h/.cpp` — `ItemStatCalculator::Calculate(base, material, prefix, postfix)` — вычисление `ItemStats` из компонентов
19. Все JSON загружаются в `main.cpp` до создания GameApp, ошибки валидации пишутся в лог
20. Изменение JSON → эффект в игре без перекомпиляции (проверено: `data/monsters.json` → новые мобы)

### Интеграция (шаги 21–25)

21. `MonsterType` enum удалён, `Monster::type` заменён на `std::string typeId` — тип загружается из JSON
22. `MonsterRenderer::Submit` принимает `std::unordered_map<std::string, Material*>` для рендера по typeId
23. Текстуры монстров загружаются динамически из JSON (`data/monsters.json` → texture → Type)
24. `PlayState::OnEnter` и `RestartGame` используют `MonsterFactory::Create` и `ItemFactory::Create*` вместо ручного конструирования
25. Все новые файлы добавлены в `Game.vcxproj` (8 .cpp + 8 .h в соответствующие секции)

### Ключевые изменения

- **Monster.h**: удалён `MonsterType` enum, добавлен `std::string typeId`, добавлен `int32_t xpReward`
- **monster_renderer.cpp**: материал выбирается по `monster.typeId` из переданной map
- **serialization.h**: `MonsterType` сериализация заменена на `typeId` (string)
- **play_state.cpp**: монстры и предметы создаются через фабрики; текстуры монстров загружаются из JSON
- **main.cpp**: `JsonDataManager::LoadAll("data")` вызывается до создания GameApp

---

## Фаза 3: Классовая система (шаги 26–40) ✅

### Выбор класса (шаги 26–33)

26. `ClassSelectionState` — экран выбора класса перед входом в игру (5 классов: Barbarian, Paladin, War Priest, Thief, Mage). ImGui-панели с названием, HP, MP, STR/DEX/CON, броней, оружием, способностями.
27. `data/classes.json` дополнен: `startingEquipment` (массив itemId) и `hitDice` (d12/d10/d8/d6).
28. `Character::charClass` — `std::string` (id из JSON), влияет на все классовые расчёты.
29. Class-modifiers для `MeleeAttack`: Barbarian +2 damage, Thief +1 damage (+1 при атаке со спины через `behind` flag).
30. Class-modifiers для `AC`: Paladin +2, Mage -2, Thief +1 (в `classAcBonus`).
31. Class-modifiers для `HP per level`: Barbarian d12, Paladin d10, War Priest d8, Thief d8, Mage d6 — `HpGainForLevel()`.
32. Проверка оружия: `CombatSystem::CanUseWeaponItem()` — сверка `allowedWeapons` из JSON.
33. Проверка брони: `CanUseArmorItem()` — сверка `allowedArmor` из JSON.

### XP и уровни (шаги 34–40)

34. `ExperienceSystem` — `AwardKill(Character&, Monster&)` — XP за убийство из `monster.xpReward`.
35. `data/level_table.json` — 20 уровней с `xpNeeded` (100, 250, 500, 800...).
36. `Character::xp`, `level`, `xpForNext` — обновляются при убийстве, проверка на level-up.
37. При level-up: +1 ATK, +2 к STR/DEX/CON на выбор, HP по hit-dice класса.
38. Level-up popup: "Level X! Choose stat to increase: STR/DEX/CON" (три кнопки, каждая начисляет бонусы).
39. XP bar в HUD: `ImGui::ProgressBar` под статами персонажа.
40. Уровень монстра скейлит статы: HP += level * 2, atkBonus += level / 2, xpReward += level * 5 (уже в `MonsterFactory`).

### Ключевые изменения

- **class_selection_state.h/.cpp** — новый файл: статический `s_pendingCharacter` передаёт персонажа в PlayState, `CreateCharacter()` собирает Character из JSON-данных класса.
- **play_state.cpp** — при `OnEnter()` проверяет `ClassSelectionState::s_pendingCharacter`; при старте через MainMenu → ClassSelection → Play. `RestartGame()` пересоздаёт персонажа через `CreateCharacter()`.
- **play_state.h** — добавлены `m_pendingLevelUp`, `m_showLevelUp` для управления popup'ом.
- **combat_system.h/.cpp** — `MeleeAttack` принимает `bool behind`; classDamageBonus/classAtkBonus используют `behind` для thief backstab (+1 damage, +1 atk).
- **direction.h** — добавлены `Opposite()` и `DirectionToTarget()` для определения атаки со спины.
- **experience_system.h/.cpp** — новые файлы: `AwardKill()`, `CheckLevelUp()`, `XpForLevel()`, `HpGainForLevel()`.
- **main_menu_state.cpp** — кнопка "Start Game" ведёт в ClassSelection, не напрямую в Play.
- **game_app.cpp** — зарегистрирован `ClassSelectionState`.
- **main.cpp** — загружает `data/level_table.json` через `JsonDataManager::LoadAll("data")`.

---

## Исправления по аудиту (Phase 4.1: проблемы 11-33) ✅

### Проблема 11: MonsterManager::At() O(1)
`game/combat/monster_manager.h/.cpp` — внутреннее хранилище заменено с `std::vector<Monster>` на `std::unordered_map<GridPosition, Monster>`. `At()` теперь O(1) по хешу позиции. `All()` возвращает `std::vector<Monster>` по значению.

### Проблема 12: Дублирование спавна
`game/states/play_state.h/.cpp` — выделены методы `spawnDefaultMonsters()` и `spawnDefaultItems()`, вызываемые из `OnEnter()` и `RestartGame()`.

### Проблема 13: camera.h — мёртвые поля
Проверено: `m_animStartPosition` и `m_animStartYaw` используются в `UpdateAnimation()` для интерполяции анимации движения/поворота. Не мёртвый код.

### Проблема 15: renderOptionsWindow() — чтение конфига
Вынесено: оба поля (`gridMoveRepeatDelay`, `renderHeight`) загружаются один раз в `OnEnter()` и `RestartGame()` через `GetGridMoveRepeatDelayFromConfig()` / `GetRenderHeightFromConfig()`. `renderOptionsWindow()` использует кэшированные `m_moveRepeatDelay` и `m_renderHeight`.

### Проблема 16: renderer.cpp — uint64_t sort key
`engine/renderer/renderer.cpp:96-106` — компаратор сортировки `DrawCommand` заменён на единый uint64_t ключ `(materialId << 32) | meshId`, устранив два условных перехода.

### Проблема 17: profiler.cpp — unordered_map
`core/profiler.h/.cpp` — добавлен `std::unordered_map<std::string, size_t> m_sampleMap`. `BeginSample()` теперь ищет сэмпл за O(1), а не O(n) через `find_if`.

### Проблема 18: resource_manager.cpp — CleanupUnused()
Комментарий обновлён, описывает необходимость миграции на `shared_ptr` для отслеживания внешних ссылок. Функция остаётся no-op безопасно.

### Проблема 19: inventory.cpp — AddResult enum
`game/combat/inventory.h/.cpp` — возвращаемое значение `Inventory::Add()` изменено с `bool` на `AddResult` (Success/Full). Вызывающий код в `play_state.cpp` обновлён на `== AddResult::Success`.

### Проблема 20: ACTION_NAMES — устранение дублирования
`game/states/play_state.h` — `GRID_ACTION_NAMES[]` объявлен единожды как `static constexpr std::string_view[]`. Оригинальные дублирующиеся статические массивы в `processEdgeActions()` и `processHeldRepeat()` удалены.

### Проблема 21: main_menu_state.h — unused fields
Удалены неиспользуемые поля `m_window` и `m_input`. Конструктор `MainMenuState` принимает только `GameStateMachine&`.

### Проблема 22: tile.h — dead file
`src/game/dungeon/tile.h` — содержимое заменено на комментарий о том, что файл мёртв. `Tile`/`TileType` не используются (в проекте `Cell` из `chunk.h`).

### Проблема 25: serialization.h — ItemDrop в .cpp
`src/game/serialization.cpp` — создан; `to_json`/`from_json` для `ItemDrop` перемещены из заголовка в .cpp. Файл добавлен в `Game.vcxproj` и `.filters`.

### Проблема 26: audio_system.cpp — LoadWAV аллокация
`LoadWAV()` оптимизирован:
- F32: `clip.samples.assign()` вместо `resize()` + `memcpy()`
- S16: `converted` копируется через `assign()` вместо предварительного `resize()`

### Проблема 28: combat_system.cpp — const
Проверено: `ClassDamageBonus`, `ClassAtkBonus`, `ClassAcBonus` уже объявлены как `static` в заголовке, не изменяют состояние. Проблема отсутствует.

### Проблема 30: profiler.h — CONCAT макросы
Макросы переименованы из `CONCAT`/`CONCAT2` в `detail_CONCAT`/`detail_CONCAT2` во избежание конфликтов с другими библиотеками.

### Проблема 33: settings_state.h — PlayerConfig
Структура `PlayerConfig` помечена комментарием `// @internal` и остаётся в заголовке (необходима как член `SettingsState` по значению).

---

## Исправления по аудиту (Phase 4.2: проблемы 18, 32, 34, 37, 38, 41, 43, 44, 45) ✅

### Проблема 18: resource_manager.cpp — CleanupUnused() с reference counting
`engine/resource_manager.h/.cpp`:
- Добавлены `Retain(key)` и `ReleaseResource(key)` для ручного управления счётчиком ссылок.
- `CleanupUnused()` удаляет ресурсы с `refCount <= 0`.
- Механизм готов для использования; существующие caller'ы не вызывают `Retain`/`ReleaseResource`, но при добавлении вызовов ресурсы будут корректно выгружаться.

### Проблема 32: json_data_manager.cpp — findById() O(1) через unordered_map
`core/json_data_manager.h/.cpp`:
- Добавлен `std::unordered_map<std::string, size_t>` для каждого JSON-массива.
- `buildIndex()` заполняет маппинг id→index после загрузки каждого файла.
- Все `GetXxx(id)` используют `getByIndex()` — O(1) доступ вместо O(n) линейного поиска.

### Проблема 34: play_state.cpp — объединение level-up блоков
`game/states/play_state.cpp`:
- Три идентичных блока (STR/DEX/CON) заменены на лямбду `applyLevelUp(statName, oldValue, newValue)`.
- Каждая кнопка вызывает лямбду с соответствующим параметром (+2 к STR/DEX/CON).

### Проблема 37: Портативная fopen/fread обёртка
`core/file_io.h/.cpp` — созданы:
- `FileReadBytes(path)` — читает файл в `std::vector<uint8_t>`
- `FileReadString(path)` — читает текстовый файл в `std::string`
- `FileWriteBytes(path, data, size)` — пишет байты в файл
- Внутренняя `openFile()` — единый `#ifdef _MSC_VER` для `fopen_s`/`fopen`.

### Проблема 38: Устранение дублирования fopen/fread/fclose
Все ручные fopen/fseek/ftell/fread/fclose заменены на `FileReadString`/`FileWriteBytes`:
- `json_data_manager.cpp:loadFile()` — переписан через `FileReadString(path)`
- `settings_state.cpp:readJsonFile()`/`writeJsonFile()` — переписаны
- `play_state.cpp:SaveGame()` — через `FileWriteBytes()`
- `play_state.cpp:LoadGame()` — через `FileReadString()`
- `play_state.cpp:renderOptionsWindow()` — через `FileReadString()`/`FileWriteBytes()`

### Проблема 41: Централизованный using json alias
`core/json_alias.h` — создан, содержит единственное `using json = nlohmann::json;`.
- Все файлы с дублирующимися `using json = ...` переведены на `#include "core/json_alias.h"`:
  - `json_data_manager.h`, `skill_manager.h`, `spell_manager.h`, `ability_manager.h`, `class_manager.h`, `item_stat_calculator.h`, `serialization.h`, `settings_state.cpp`

### Проблема 43: Унифицированный DataManager
`core/data_manager.h/.cpp` — создан единый `DataManager` struct вместо 4 статических классов:
- Содержит методы: `GetClass()`, `ApplyStartingStats()`, `GetSkill()`, `GetSkillsForClass()`, `GetSkillsForClassAtLevel()`, `GetPassiveSkillsForClassAtLevel()`, `GetSkillIdsForClassAtLevel()`, `GetNewSkillIdsForLevel()`, `GetAbility()`, `GetAbilitiesByClass()`, `GetSpell()`, `GetSpellsByClass()`, `CalculateItemStats()`
- Определения типов `Skill`, `ActionSlot`, `ItemStats` перенесены в `data_manager.h`
- Старые заголовки (`skill_manager.h`, `spell_manager.h`, `ability_manager.h`, `class_manager.h`, `item_stat_calculator.h`) — теперь просто `using X = DataManager;`
- Старые `.cpp` файлы опустошены (имплементация в `data_manager.cpp`)
- Все caller'ы работают без изменений через type alias'ы

### Проблема 44: GameStateMachine — m_isPaused флаг
`engine/game_state.h/.cpp`:
- Добавлен `bool m_isPaused = false`.
- `applyPush()`: устанавливает `m_isPaused = true` перед пушем, сбрасывает в `false` после.
- `applyPop()`: сбрасывает `m_isPaused = false` перед `OnResume()`.
- При исключении в `OnEnter()`: `m_isPaused` корректно сбрасывается.
- `OnPause()`/`OnResume()` теперь корректны при любом размере стека.

### Проблема 45: Monster entity-ID
`game/combat/monster.h`:
- Добавлено `uint32_t id = 0` (первое поле структуры).

`game/combat/monster_manager.h/.cpp`:
- Добавлен `uint32_t m_nextId = 1` (автоинкремент).
- `Spawn()`: присваивает `monster.id = m_nextId++` перед вставкой.
- Добавлены `FindById(uint32_t id)` — const и non-const версии, проход по всем монстрам в `m_monsters`.

---

## Исправления по аудиту (Phase 4: проблемы 2,3,4,6,7,8,9,10) ✅

### Проблема 2: State machine exception safety
`engine/game_state.cpp` — `applyPush()` и `applyReplace()`:
- При исключении в `OnEnter()` теперь вызывается `OnResume()` на предыдущем состоянии, восстанавливая корректный жизненный цикл стека состояний.

### Проблема 3: Дублирование `hp = maxHp` в level-up
`game/states/play_state.cpp:1598-1641` — удалено второе присваивание `m_character.hp = m_character.maxHp` в каждой из трёх веток (STR/DEX/CON).

### Проблема 4: `AudioSystem::Stop()` — деструктивная перезапись стрима
`engine/audio_system.cpp` — `Stop()` теперь только удаляет клип из `m_playing`, не очищая и не перезаливая весь SDL-аудиострийм. Данные в стриме доигрываются естественным образом, синхронизация не теряется.

### Проблема 6: Character — инкапсуляция
`game/combat/character.h/.cpp`:
- `Character` изменён с `struct` на `class` с `private`-полями (префикс `m_`).
- Добавлены геттеры/сеттеры (`GetHp()`, `SetMaxHp()`, `GetStr()`, и т.д.).
- Добавлены методы-мутаторы: `TakeDamage()`, `Heal()`, `SpendMp()`, `RestoreMp()`, `AddXp()`.
- Сериализация — через `friend`-функции `to_json`/`from_json`.
- Обновлены все потребители: `play_state.cpp`, `class_selection_state.cpp`, `combat_system.cpp`, `experience_system.cpp`, `serialization.h`.

### Проблема 7: `static Character` в ClassSelectionState
`game/states/class_selection_state.h/.cpp`:
- Удалён `static Character s_pendingCharacter`.
- `Character m_pendingCharacter` перенесён в `GameApp`.
- `ClassSelectionState` получает `Character&` через конструктор и пишет в него при Confirm.
- `PlayState` получает `Character*` через конструктор и читает при `OnEnter()`, затем обнуляет.
- `CreateCharacter()` вынесена в свободную функцию `CreateCharacterFromClass()` (доступна из `PlayState::RestartGame()`).

### Проблема 8: Inventory MAX_ITEMS консистентность
`game/combat/inventory.h/.cpp`:
- Заменён `std::vector<Item> m_items` на `std::array<Item, MAX_ITEMS> m_items` + `size_t m_size`.
- Лимит в 32 элемента гарантирован на уровне типа, а не только проверкой в `Add()`.
- Операции `Remove()`/`Clear()`/`Get()`/`Size()` переписаны на работу с `m_size`.

### Проблема 9: build.bat — мусорные .o в линковке
`build.bat:142-146`:
- Линковка теперь собирает .o файлы из списка исходников (`ALL_SRCS`), а не рекурсивным поиском по `_obj\gcc\*`.
- Старые .o от удалённых/переименованных файлов больше не попадают в линковку.

### Проблема 10: OGG загрузка — null check + конверсия
`engine/audio_system.cpp`:
- Добавлена проверка `output == nullptr` после `stb_vorbis_decode_filename`.
- Конверсия S16→F32 переписана с `std::transform` и предвычисленным `inv32768` вместо цикла с поэлементным делением.

---

## Фаза 4: Система снаряжения (шаги 56–90) ✅

### Шаг 56–57: EquipmentSlot + Equipment
- `EquipmentSlot` enum в `item.h`: Weapon, Shield, Head, Body, Hands, Feet, Ring, Amulet
- `Item` struct расширен полями статов: `damageMin/Max`, `ac`, `atkBonus`, `strBonus`, `dexBonus`, `conBonus`, `hpBonus`, `mpBonus`, `elementDamageMin/Max`, `elementType`, `lifeStealPercent`, `slot`
- `Equipment` класс в `game/combat/equipment.h/.cpp`: `std::array<Item, 8>` с `m_occupied[]`, методы `Equip()`, `Unequip()`, `GetTotalStats()`, `Clear()`, сериализация

### Шаг 58–61: Character equipment + Equip/Unequip + GetEquippedStats
- `Character` добавлен `Equipment m_equipment` + getter `GetEquipment()`
- `Character::GetEquippedAc/AtkBonus/DamageMin/DamageMax()` — базовый стат + оборудование
- `Equipment::Equip()` возвращает предыдущий предмет, `Unequip()` в инвентарь

### Шаг 62: CombatSystem использует equipped stats
- `combat_system.cpp`: все вызовы `GetAtkBonus()` → `GetEquippedAtkBonus()`, аналогично для `GetAc()`, `GetDamageMin/Max()`

### Шаг 63: UI экипировки
- Окно "Equipment (E)" с 8 слотами, тултипы с полными статами, кнопка Unequip
- Total Equipment Stats внизу окна
- Кнопка "Equip" в инвентаре для предметов с ненулевым `slot`
- Хоткей E для открытия/закрытия

### Шаг 64–73: Prefix-Material-Postfix генерация
- `ItemGenerator` в `game/data/item_generator.h/.cpp`: `GenerateWeapon()`, `GenerateArmorItem()`, `GenerateAccessory()`, `GenerateEquipment()`
- Алгоритм: выбрать base (random из JsonDataManager) → префикс/материал/постфикс по tierCap (уровень игрока / 3)
- `determineRarity()` по maxTier из трёх частей
- Имя: "Prefix Material BaseName of Postfix"
- Использует `DataManager::CalculateItemStats()` для финальных статов
- ElementDamage и lifeSteal из префикса/постфикса

### Шаг 74–82: Расчёт статов (существующий)
- `ItemStats` и `DataManager::CalculateItemStats()` уже существовали, используются напрямую
- ElementDamage/lifeSteal добавляются в `ItemGenerator`

### Шаг 83–90 (UI — частично)
- Окно экипировки с просмотром/тултипами/снятием
- Equip через кнопку в инвентаре
- **Не сделано**: drag-n-drop, сравнение статов (зелёным), визуальное отображение на персонаже, `GetEquippableItems()` фильтр

---

## ✅ Фаза 6: Расширение боевой системы (шаги 131–165)

> Цель: дальний бой, AoE, статус-эффекты, группы монстров, тактическая глубина.

### ✅ Ranged Combat (шаги 131–138)

131. ✅ `WeaponType::Range` в weapon_types.json: melee (1 клетка), ranged (3–5 клеток)
132. ✅ Ranged оружие: Bow (3 клетки), Crossbow (4 клетки), Staff (2 клетки, магия)
133. ✅ Атака с ranged оружием: выбор цели на дистанции через targeting
134. ✅ Стрельба: проверка на преграду (стена или другой монстр блокирует)
135. ✅ Стрелы/болты: ItemType::Arrow/Bolt, расходуются при стрельбе
136. ✅ Для магии: staff — фокус, увеличивает damage на 50%, не расходует mp
137. ✅ Монстры тоже могут быть ranged: лучники, маги — атакуют с дистанции
138. ✅ Ranged атака в ближнем бою: штраф -2 atkBonus (если монстр вплотную)

### ✅ AoE and Targeting (шаги 139–146)

139. ✅ `TargetingMode` — enum: Single, Beam, AoE_3x3, AoE_5x5, Cross, Line
140. ✅ Для Area-заклинаний: выбор направления (как сейчас взгляд) или выбор клетки
141. ✅ Если AoE — показать сетку подсветки
142. ✅ `AoeTarget` — структура: центр, радиус, поражённые клетки
143. ✅ Friendly fire: если в AoE нет монстров — предупреждение, но кастануть можно
144. ✅ Для Line/Beam: проверка каждой клетки на пути до упора в стену
145. ✅ Монстры с AoE: огненные шары, ядовитые облака
146. ✅ Урон по площади: каждый монстр в зоне получает урон, спасбросок на половину DEX

### ✅ Status Effects (шаги 147–157)

147. ✅ `StatusEffectDef` — структура: id, name, type, duration, value, tickInterval
148. ✅ Типы эффектов: DamageOverTime (огонь/яд), Slow (львиная), Stun, Blind, Regen, Shield, Invisibility, Silence
149. ✅ `Character::m_activeEffects` — вектор активных статусов, обновляется каждый ход
150. ✅ `StatusSystem` — применение эффектов: тик урона, снятие по истечении длительности
151. ✅ Применение: из spells.json "Fireball" добавляет Burn (DoT: 2 урона/ход, 3 хода)
152. ✅ "Ice Shard": добавляет Slow (хода в 2 раза длиннее)
153. ✅ Очищение: Paladin "Cleanse" снимает все негативные эффекты
154. ✅ Резисты: монстры в data/monsters.json имеют resistances [fire, ice, poison]
155. ✅ Если монстр резистит тип — эффект не накладывается или короче
156. ✅ Immunity: некоторые монстры иммуны к определённым эффектам
157. ✅ UI эффектов: окно Status Effects (U) со списком активных эффектов

### ✅ Enemy Groups (шаги 158–165)

158. ✅ `MonsterGroup` — структура: {positions[], monsterIds[]}, группа монстров в одной области
159. ✅ Spawn groups: `EncounterTemplate` — группы монстров для генерации (JSON-ready)
160. ✅ Инициатива группы: все монстры группы ходят одновременно
161. ✅ Агро: если игрок атакует одного монстра из группы — вся группа агрится
162. ✅ Монстры могут патрулировать группой (следуют за лидером)
163. ✅ Разные роли в группе: tank (высокий AC, спереди), dps (сзади), healer (лечит)
164. ✅ Боссы: уникальные монстры с именем, особыми способностями, большим HP
165. ✅ Encounter table: на каждом этаже подземелья свои группы монстров (через weight/minFloor/maxFloor)

---

## ✅ Фаза 7: Крафт — Ресурсы и рецепты (шаги 166–173)

> Цель: кузнечное дело, алхимия и готовка с рецептами из JSON.

166. ✅ `data/resources.json` — 22 ресурса (Iron Ore, Coal, Herb, Flower, Meat, Mushroom, Water, Sulfur, Leather, Bone, Essences, etc.)
167. ✅ `data/recipes.json` — 11 рецептов: smelt_iron, healing_potion, mana_potion, cook_meat, herb_tea, fruit_juice, bread, fire_bomb, poison_bomb, leather_armor, iron_sword
168. ✅ `data/categories.json` — 4 категории: Weaponsmith, Armorsmith, Alchemy, Cooking
169. ✅ `CraftingRecipe` — структура: id, name, category, stationType, skillReq, ingredients[], result
170. ✅ `CraftingSystem` — проверка ингредиентов → удаление → создание результата через ItemFactory
171. ✅ Станции крафта: stationType в recipe (anvil, cauldron, furnace, campfire)
172. ✅ Готовка: рецепты с stationType "campfire" (cook_meat, herb_tea, fruit_juice, bread)
173. ✅ Рецепты открыты по умолчанию (UnlockAllRecipes), система unlock готова к расширению

### Ключевые изменения

- `src/game/crafting/crafting_recipe.h` — новая структура с `CraftingRecipe`, `CraftingIngredient`, `CraftingResult`, статический `FromJson()`
- `src/game/crafting/crafting_system.h/.cpp` — новый класс: `LoadRecipes()`, `LoadCategories()`, `CanCraft()`, `Craft()`, `GetRecipesByCategory()`, `GetAvailableRecipes()`, `UnlockRecipe()`, `IsRecipeUnlocked()`
- `src/game/combat/item.h` — добавлено поле `std::string itemId` для идентификации предметов при крафте
- `src/game/data/item_factory.cpp` — `CreateBase()` заполняет `itemId`
- `src/core/json_data_manager.h/.cpp` — загрузка `resources.json`, `recipes.json`, `categories.json` (опциональные)
- `src/core/data_manager.h/.cpp` — добавлены методы `GetResourceData()`, `GetRecipeData()`, `AllResources()`, `AllRecipes()`, `AllCategories()`
- `src/game/states/play_state.h/.cpp` — `CraftingSystem m_craftingSystem`, `renderCraftingWindow()` (C), интеграция в OnEnter/Render/HandleEvent/RestartGame
- `src/game/serialization.h` — `itemId` в `to_json`/`from_json` для Item
- `src/Game.vcxproj` — добавлены `crafting_system.cpp`, `crafting_recipe.h`, `crafting_system.h`

---

## ✅ Фаза 8: Кузнечное дело и улучшение оружия (шаги 174–182)

> Цель: кузнечное ремесло — создание, улучшение, заточка, ремонт и модификация оружия.

174. ✅ **Создание оружия** — `CraftingSystem::CreateWeapon()`: baseType (dagger/sword/axe и т.д.) + material (iron/steel/silver/mithril/adamant) → готовое оружие с именем "Steel Sword", рассчитанными статами через `DataManager::CalculateItemStats()`. Расходуется 1 Iron Ingot.
175. ✅ **Улучшение оружия** — `CraftingSystem::UpgradeWeapon()`: экипированное оружие + материал → добавляет префикс "Steel" к имени, +tier к damage/atk. Upgrade только с улучшением (tier выше текущего). Расходуется 1 Iron Ingot (через `HasIngredients`). XP: 15 + matTier*5.
176. ✅ **Заточка** — `CraftingSystem::SharpenWeapon()`: weapon + whetstone → +1 damage (макс +3). Имя суффикс " (sharpened +N)". Расходуется 1 Whetstone. XP: 20.
177. ✅ **Постфикс (зачарование)** — `CraftingSystem::AddPostfix()`: weapon + essence → добавляет "of Fire/Ice/Poison" с elementDamage 3-5. Старый постфикс удаляется. XP: 25.
178. ✅ **Эссенции из монстров** — в `combat_handler.cpp::onKill()`: монстры с полем `"element"` дропают essences (fire_essence, ice_essence, poison_essence). Добавлены: Fire Elemental (fire), Ice Elemental (ice), Poison Slime (poison).
179. ✅ **Ремонт** — `CraftingSystem::RepairItem()`: восстанавливает `durability` до `maxDurability`. XP: 5.
180. ✅ **Прочность оружия** — `Item` расширен полями `durability`/`maxDurability` (int32_t). `reduceWeaponDurability()` в CombatHandler: при каждом ударе −1 durability. При 0 — оружие ломается (`breaks!` в лог).
181. ✅ **Плавка в инготы** — рецепты в recipes.json: `smelt_iron` (2 iron_ore + coal → 1 iron_ingot), `smelt_steel` (2 iron_ingot + 2 coal → 1 steel_ingot). Добавлены `steel_ingot`, `coal`, `whetstone` в `items_base.json`.
182. ✅ **Кузнечный уровень** — `CraftingSystem::m_smithingXp` растёт с каждой операцией (Create: 10-25, Upgrade: 15-30, Sharpen: 20, Postfix: 25, Repair: 5). Каждые 100 XP → +1 уровень. Отображается в UI.

### Armorsmith (шаги 183–190)

183. Аналогично weaponsmith, но для брони: baseType + material + ingot
184. Создание брони каждого типа (Helm, Body, Shield, Hands, Feet)
185. Улучшение брони: +material → +AC
186. Добавление постфикса: броня + essence → of Protection (+AC), of Vitality (+HP)
187. Починка брони: броня + material → восстановление прочности
188. Прочность брони: при получении урона броня теряет прочность
189. Зачарование: броня + scroll + essence → добавляет магический эффект
190. Уровень бронирования растёт отдельно от кузни

### Ключевые изменения

- `src/game/combat/item.h` — добавлены `maxDurability`, `durability`, `sharpnessLevel`, `prefixId`, `postfixId`, `materialId`
- `src/game/crafting/crafting_system.h/.cpp` — добавлены 6 методов оружейника: `CreateWeapon()`, `UpgradeWeapon()`, `SharpenWeapon()`, `AddPostfix()`, `RepairItem()`, `AddSmithingXp()`, `GetSmithingLevel()`, `IsWeapon()`, `HasIngredients()`; поля `m_smithingXp`, публичный `m_craftingLevel`
- `src/game/combat/combat_handler.h/.cpp` — добавлены `reduceWeaponDurability()`, поле `m_itemHandler`; `onKill()` расширен для дропа лута (из drops монстра) и эссенций (по element); Init принимает `ItemHandler&`
- `src/game/combat/item_handler.h/.cpp` — публичный `Drops()` возвращает `std::vector<ItemDrop>&`
- `src/game/states/play_state.h/.cpp` — `renderWeaponsmithOperations()` с UI для создания оружия (Combo базового типа + материала), upgrade, sharpen, postfix, repair; Init передаёт `m_itemHandler` в `CombatHandler`
- `src/game/serialization.h` — новые поля Item в `to_json`/`from_json` (maxDurability, durability, sharpnessLevel, prefixId, postfixId, materialId)
- `bin/data/items_base.json` — добавлены: `iron_dagger`, `steel_dagger`, `iron_sword`, `steel_sword`, `iron_axe`, `steel_axe`, `ice_essence`, `poison_essence`, `steel_ingot`, `coal`, `whetstone`
- `bin/data/monsters.json` — добавлены Ice Elemental (ice), Poison Slime (poison); Fire Elemental получил `"element": "fire"`; новые монстры имеют drops с essences
- `bin/data/recipes.json` — расширен 8 рецептами: smelt_steel, craft_iron_sword/dagger/axe, craft_steel_sword/axe/dagger, craft_iron_shield

---

## ✅ Фаза 7: Алхимия (шаги 191–198)

191. ✅ **Зелья** — рецепты `healing_potion` (herb×2 + water + bottle), `mana_potion` (flower×3 + water + bottle), `antidote_potion` (herb×2 + coal + bottle) в `recipes.json`. Создаются через `CraftingSystem::Craft()`.
192. ✅ **Бомбы** — рецепты `fire_bomb` (sulfur×2 + fire_essence + bottle), `poison_bomb` (sulfur×2 + poison_essence + bottle), `ice_bomb` (sulfur + herb + bottle). ItemType::Bomb в `items_base.json`. Использование через инвентарь.
193. ✅ **Эссенции из дропов** — рецепты `craft_essence_fire/ice/poison`: slime_glob×2 + catalyst → essence. Дают альтернативный способ получения эссенций.
194. ✅ **Catalyst** — добавлен в `items_base.json` и `resources.json`. Используется в рецептах эссенций, доступен через крафт/дроп.
195. ✅ **Яды** — рецепт `poison_vial` (poison_herb×2 + bottle). UI: кнопка "Apply Poison Vial" в `renderAlchemyOperations()` — наносит элемент poison на оружие (+1d4).
196. ✅ **Масла** — рецепты `oil_fire` (oil + fire_essence) и `oil_ice` (oil + ice_essence). UI: Combo выбора + "Apply Oil" — наносит элемент fire/ice на оружие (+2d4).
197. ✅ **Эликсиры** — рецепты `elixir_strength/dexterity/constitution` (herb/flower/mushroom×3 + gem + bottle). Создаются как ItemType::PotionHeal с бонусом +2 к статам.
198. ✅ **Алхимия прокачивается** — `CraftingSystem::m_alchemyXp`, `AddAlchemyXp()`, `GetAlchemyLevel()`. XP начисляется при каждом `Craft()` с категорией "alchemy" и при операциях нанесения ядов/масел. Отображается в UI.

## ✅ Фаза 7: Готовка (шаги 199–204)

199. ✅ **Еда** — рецепты `cook_meat` (meat + spice), `cooked_fish` (fish + spice) через `CraftingSystem::Craft()`. Создают ItemType::Food с hungerRestore.
200. ✅ **Супы** — рецепты `mushroom_soup` (mushroom×2 + herb + water), `vegetable_soup` (vegetable×2 + spice + water). Восстанавливают hunger 25-30.
201. ✅ **Выпечка** — рецепт `bread` (flour×2 + egg + honey) уже существовал. Восстанавливает hunger 30.
202. ✅ **Напитки** — рецепт `fruit_juice` (fruit×2 + water) уже существовал. Добавлен `honey_mead` (honey×2 + water + fruit).
203. ✅ **Особые блюда** — рецепт `special_dish` (meat×2 + mushroom + spice×2). Самое сильное восстановление hunger (+60).
204. ✅ **Уровень готовки** — `CraftingSystem::m_cookingXp`, `AddCookingXp()`, `GetCookingLevel()`. XP за каждый `Craft()` с категорией "cooking". Отображается в UI.

## ✅ Фаза 7: Сбор ресурсов (шаги 205–210)

205. ✅ **Resource nodes** — `Cell::isResourceNode` уже был в структуре. В `GenerateTestRoom()` расставлены 8 нод: mining (3), herbalism (3), fishing (1), gem (1).
206. ✅ **Mining** — требует `pickaxe` в слоте Weapon. Добывает: iron_ore, coal, silver_ore. Проверка через `m_character.GetEquipment().Get(EquipmentSlot::Weapon)`.
207. ✅ **Herbalism** — без инструмента. Добывает: herb, flower, mushroom, poison_herb.
208. ✅ **Looting** — уже реализован в `CombatHandler::onKill()` (шаг 178): монстры дропают ресурсы (essences, leather, bones, meat) через JSON-поле `drops`.
209. ✅ **Fishing** — требует `fishing_rod` в слоте Weapon. Добывает `fish`. Эксплуатирует те же resource nodes с проверкой инструмента.
210. ✅ **Инструменты** — `pickaxe` и `fishing_rod` добавлены в `items_base.json`. Тип weapon, слот Weapon. UI: при попытке собрать без инструмента — сообщение в лог.

### Ключевые изменения (шаги 191–210)

- `src/game/crafting/crafting_system.h/.cpp` — добавлены `m_alchemyXp`, `m_cookingXp`, `AddAlchemyXp()`, `GetAlchemyLevel()`, `AddCookingXp()`, `GetCookingLevel()`, `removeItems()` публичный. XP начисляется в `Craft()` по категории.
- `src/game/states/play_state.h/.cpp` — добавлены `renderAlchemyOperations()` (нанесение ядов/масел, алхимия XP), `renderCookingOperations()` (готовка XP), `processGather()` (сбор ресурсов с проверкой инструментов). Вызов gather добавлен в Space-обработчик перед pickup.
- `src/game/dungeon/dungeon.cpp` — `GenerateTestRoom()` расставляет 8 resource nodes разных типов.
- `src/game/data/item_factory.cpp` — поддержка типов `bomb`, `food`, `material` в `CreateBase()`.
- `bin/data/items_base.json` — добавлены 20 предметов: antidote_potion, fire/ice/poison_bomb, catalyst, poison_vial, oil_fire/oil_ice, elixir_strength/dex/con, cooked_meat/fish, mushroom/vegetable_soup, honey_mead, special_dish, pickaxe, fishing_rod, silver/gold_ore, gem, fish, poison_herb, oil, salt.
- `bin/data/resources.json` — добавлены 6 ресурсов: silver_ore, gold_ore, gem, poison_herb, oil, catalyst, salt, fish.
- `bin/data/recipes.json` — добавлены 18 рецептов: antidote_potion, poison_vial, oil_fire/oil_ice, craft_essence_fire/ice/poison, elixir_strength/dex/con, mushroom/vegetable_soup, honey_mead, special_dish, cooked_fish.

---

## ✅ Фаза 8: Система голода и еда (шаги 211–219)

> Цель: система голода, еда как необходимость, баффы от приготовленной еды.

### ✅ Hunger System (шаги 211–219)

211. ✅ `Character::hunger` — шкала 0–100 (`m_hunger`), геттеры/сеттеры `GetHunger()`, `SetHunger()`, `AddHunger()`, `ConsumeHunger()`. Уменьшается на 1 каждый ход в `processTurnWaiting()`. Константы `MAX_HUNGER=100`, `HUNGER_WARNING=30`, `HUNGER_STARVING=15`.

212. ✅ Сообщения при порогах: "You feel hungry" при hunger < 30, "You are starving! (-1 ATK, -1 AC)" при hunger < 15. Однократный показ через флаги `m_hungerWarningShown`/`m_hungerStarvingShown`.

213. ✅ Штраф при hunger < 15: `GetHungerAtkPenalty()` и `GetHungerAcPenalty()` возвращают -1. Включены в `GetEquippedAtkBonus()` и `GetEquippedAc()`. Отображается в HUD.

214. ✅ При hunger = 0: "You collapse from hunger! (-1 HP)" каждый ход. При HP ≤ 0 — GameOver.

215. ✅ Еда восстанавливает hunger через кнопку "Use" в инвентаре. Значение `hungerRestore` читается из `items_base.json`. bread +30, mushroom_soup +30, cooked_meat +40, special_dish +60 и т.д.

216. ✅ Голод не убивает мгновенно — даёт время найти/приготовить еду (только -1 HP/ход при hunger=0).

217. ✅ На старте: `m_hunger = 80` (сыт). Сброс в `RestartGame()`.

218. ✅ Raw meat (subtype `raw_meat`): при употреблении 30% шанс отравления (poison status 3 хода) через `StatusSystem::ApplyEffect()`.

219. ✅ Приготовленная еда даёт баффы без риска отравления:
    - `cooked_meat` → "Well Fed": +1 ATK, 20 ходов
    - `soup` → "Warm Soup": +1 MP regen, 30 ходов
    - `meal` → "Full Meal": +2 ATK, +2 AC, 30 ходов
    - `drink` → "Tipsy": +1 ATK, -1 AC, 15 ходов (алкоголь)

### Ключевые изменения

- `src/game/combat/character.h` — добавлены: `GetHungerAtkPenalty()`, `GetHungerAcPenalty()`, `FoodBuff` struct, `m_activeBuffs`, `ApplyFoodBuff()`, `TickBuffs()`, `GetBuffsAtkBonus()`, `GetBuffsAcBonus()`. Изменены `GetEquippedAtkBonus()`, `GetEquippedAc()`, `GetMpRegenPerTurn()` для включения штрафов/баффов.
- `src/game/states/play_state.h` — добавлены `m_hungerWarningShown`, `m_hungerStarvingShown`.
- `src/game/states/play_state.cpp` — `processTurnWaiting()`: hunger warning сообщения, штраф -1 ATK/AC, -1 HP при hunger=0, тик баффов. `renderInventoryWindow()`: поддержка `ItemType::Food` (Use — восстановление hunger, poison для raw meat, баффы для cooked food). `renderHungerIndicator()`: отображение штрафа. `renderStatusEffectsWindow()`: отображение активных Food Buffs. `RestartGame()`: сброс hunger/баффов.
- `src/game/serialization.h` — `Character::to_json/from_json`: сериализация `m_hunger` и `m_activeBuffs`. Добавлены `to_json`/`from_json` для `Character::FoodBuff`.
- `src/game/states/play_state.cpp` — добавлен `#include "game/combat/dice.h"` для броска poison-шанса.
