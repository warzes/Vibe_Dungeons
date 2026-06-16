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
