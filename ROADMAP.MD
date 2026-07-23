# Roadmap: Dungeon Crawlers — переписывание с нуля

Дорожная карта для переписывания игры с черновика (AI-кода) на чистовик.
Каждый шаг — законченная итерация, после которой проект компилируется и запускается.

**Стартовая точка**: скелет проекта — `build.bat`/`Game.vcxproj` настроены, `stdafx.h` подключает все зависимости, `main.cpp` пустой (только `main()` с `return 0`).

---

## Фаза 0: Инфраструктура и ядро

### Шаг 0.1 — Логгер и исключения
- Скопировать `core/logger.h/.cpp` — потокобезопасный логгер с уровнями (Info/Error/Fatal)
- Скопировать `core/exception.h` — `EngineException` с `source_location`
- Скопировать `core/scoped_sdl.h` — RAII для `SDL_Init`/`SDL_Quit`
- Подключить `logger.h` в `main.cpp`, вывести приветственное сообщение
- **Результат**: приложение запускается, инициализирует SDL, пишет в консоль, корректно завершается

### Шаг 0.2 — Профайлер
- Скопировать `core/profiler.h/.cpp` — scoped-профайлер с накоплением статистики
- **Результат**: профайлер доступен, можно оборачивать участки кода в `PROFILE_SCOPE`

### Шаг 0.3 — Окно и OpenGL контекст
- Скопировать `engine/window.h/.cpp` — создание SDL-окна, swap buffers
- Скопировать `engine/gl_context.h/.cpp` — инициализация OpenGL через glad, MSAA
- Подключить в `main.cpp`: создать окно, создать GL-контекст, очистить экран цветом
- **Результат**: открывается окно 1280x720 с цветным фоном

### Шаг 0.4 — DeltaTime
- Скопировать `engine/delta_time.h/.cpp` — измерение времени кадра
- **Результат**: `DeltaTime` корректно считает `Seconds()` между кадрами

### Шаг 0.5 — Ввод (InputManager)
- Скопировать `engine/input_manager.h/.cpp` — состояние клавиатуры/мыши, привязки действий
- Добавить обработку SDL-событий в main loop (poll + begin frame)
- **Результат**: можно определить `IsKeyDown(SDL_SCANCODE_W)`, `IsActionPressed("Attack")`

### Шаг 0.6 — Шейдеры
- Скопировать `engine/renderer/shader.h/.cpp` — компиляция/линковка GLSL, кэш uniform-ов
- Скопировать `engine/renderer/shader_sources.h` — встроенные шейдеры (dungeon, screen)
- **Результат**: можно загрузить шейдер из строки и установить uniform

### Шаг 0.7 — Текстуры
- Скопировать `engine/renderer/texture.h/.cpp` — загрузка 2D-текстур через stb_image
- Скопировать `core/file_io.h/.cpp` — чтение/запись файлов (WASM-safe через `fopen`)
- Скопировать `core/json_alias.h` — алиас для `nlohmann::json`
- Скопировать `core/json_data_manager.h/.cpp` — загрузчик JSON-файлов из `data/`
- **Результат**: можно загрузить PNG-текстуру и привязать к шейдеру

### Шаг 0.8 — Материалы и меш
- Скопировать `engine/renderer/material.h/.cpp` — связка шейдер + текстура
- Скопировать `engine/renderer/mesh.h/.cpp` — VAO/VBO/EBO, загрузка OBJ/GLTF
- Скопировать `core/aabb.h` — axis-aligned bounding box
- **Результат**: можно создать материал и загрузить меши

### Шаг 0.9 — Камера (сеточное движение)
- Скопировать `game/grid_position.h` — `GridPosition`
- Скопировать `game/direction.h` — `Direction` + хелперы
- Скопировать `engine/renderer/camera.h/.cpp` — перспективная камера + сеточное движение с анимацией
- Написать простой цикл: WASD двигает камеру по сетке
- **Результат**: камера حرکت мгновенно по клеткам 1x1, 90° повороты, плавная анимация (0.2s)

### Шаг 0.10 — FBO и screen quad
- Скопировать `engine/renderer/framebuffer.h/.cpp` — FBO с цветовым + depth-аттачментом
- Реализовать screen quad в `main.cpp` (или временном классе) — рендер FBO в окно
- Добавить ретро-режим (низкое разрешение FBO → растягивается на экран)
- **Результат**: рендер идёт в FBO → отображается на экране через quad

### Шаг 0.11 — Renderer
- Скопировать `engine/renderer/renderer.h/.cpp` — инстансированный батч-рендерер
- Скопировать `engine/renderer/frustum.h/.cpp` — frustum culling
- **Результат**: рендерер принимает mesh + материал + матрицы и рисует меши

### Шаг 0.12 — Machine состояний игры
- Скопировать `engine/game_state.h/.cpp` — стек состояний с Push/Pop/Replace
- **Результат**: можно зарегистрировать состояние, переключаться между ними

### Шаг 0.13 — ResourceManager
- Скопировать `engine/resource_manager.h/.cpp` — кэш шейдеров, текстур, материалов по имени
- **Результат**: `GetOrCreateShader()`, `LoadTexture()`, `GetOrCreateMaterial()` работают

### Шаг 0.14 — ImGui
- Скопировать `engine/renderer/debug_renderer.h/.cpp` — линии/фигуры для дебага
- Подключить ImGui в main loop (NewFrame → Render → конец кадра)
- Скопировать `engine/audio_system.h/.cpp` (заглушка)
- **Результат**: ImGui окна рисуются поверх 3D-сцены, можно вывести FPS

### Шаг 0.15 — GameApp (главный цикл)
- Скопировать `game/game_app.h/.cpp` — класс `GameApp` с циклом `Run() → Frame()`
- Перенести туда: окно, GL-контекст, ввод, FBO, renderer, state machine, ImGui
- В `main.cpp` оставить только: `JsonDataManager::LoadAll()`, создание `ScopedSDL`, `GameApp`, `app.Run()`
- Добавить фиксированный таймстеп для физики (accumulator pattern)
- **Результат**: полноценный main loop с FBO, ImGui, fixed timestep

---

## Фаза 1: Подземелье и движение

### Шаг 1.1 — Chunk (ячейка карты)
- Скопировать `game/dungeon/chunk.h` — `Cell` + `Chunk` (35×35)
- **Результат**: `Cell` с флагами `hasFloor`, `isWall`, `isLockedDoor`, `isOpenDoor`, `isTrap` и т.д.

### Шаг 1.2 — Генератор подземелья
- Скопировать `game/dungeon/dungeon_generator.h/.cpp` — 4 алгоритма (rooms, random walk, cellular automata, grid)
- Скопировать `game/dungeon/dungeon.h/.cpp` — обёртка над Chunk + проверка проходимости
- **Результат**: `Dungeon::Generate(seed)` создаёт карту с комнатами, коридорами, дверями, ловушками, секретными стенами, ресурсами, лестницами

### Шаг 1.3 — Рендер подземелья
- Скопировать `game/dungeon/dungeon_renderer.h/.cpp` — геометрия пола/стен/потолка инстансами
- Загрузить текстуры (`data/texture_08.png`, `texture_10.png`, `texture_11.png`)
- **Результат**: видно подземелье от первого лица, стены блокируют обзор, пол/потолок текстурированы

### Шаг 1.4 — PlayState (скелет)
- Скопировать `game/states/play_state.h/.cpp` — только часть `OnEnter`/`Update`/`RenderScene`
- Зарегистрировать "Play" состояние в `GameApp`
- Подключить `PlayState` вместо MainMenu
- **Результат**: при запуске сразу попадаем в подземелье, камера стоит на старте

### Шаг 1.5 — Сеточное движение в подземелье
- Реализовать в `PlayState` обработку действий `GridMoveForward/Backward`, `TurnLeft/Right`, `StrafeLeft/Right`
- Привязать клавиши WASDQE + Space
- Добавить проверку проходимости (`IsWalkable`)
- Добавить `MoveForward()`, `MoveBackward()`, `MoveLeft()`, `MoveRight()` в камеру
- **Результат**: игрок ходит по подземелью, не проходит сквозь стены, 90° повороты

### Шаг 1.6 — HUD (信息面板)
- Добавить в `PlayState::Render()` окно ImGui с управлением (W/S, A/D, Q/E, Space)
- Показать размер карты и текущую позицию камеры
- **Результат**: видим HUD с подсказками управления и координатами

### Шаг 1.7 — Debug камера (F1)
- Переключение между сеточной и свободной камерой (WASD + мышь)
- **Результат**: F1 включает free-fly камеру, ещё раз F1 — возвращает на сетку

### Шаг 1.8 — Карта (Tab)
- Отрисовать карту подземелья в ImGui окне (тёплые стены, тёмный пол)
- Показать позицию игрока зелёным кружком
- **Результат**: Tab показывает миникарту

---

## Фаза 2: Персонаж и повороты

### Шаг 2.1 — Character (базовый)
- Скопировать `game/combat/character.h` — `Character` с базовыми статами (HP, AC, ATK, STR, DEX, CON, класс, уровень)
- Скопировать `game/combat/character.cpp` — `TakeDamage`, `Heal`, `SpendMp`, `RestoreMp`
- **Результат**: `Character` создаётся со статами по умолчанию (barbarian)

### Шаг 2.2 — GameMode и TurnManager
- Скопировать `game/game_mode.h` — `enum class GameMode`
- Скопировать `game/turn_manager.h/.cpp` — управление ходами
- Добавить в `PlayState`: движение消耗 1 ход, после хода монстры атакуют (пока заглушка)
- **Результат**: после каждого шага срабатывает `EndPlayerTurn()`, `ApplyRegen()`

### Шаг 2.3 — Combat log
- Скопировать `game/ui/combat_log.h/.cpp` — лог с цветными сообщениями
- Добавить окно "Combat Log" справа в `PlayState::Render()`
- **Результат**: видим лог действий справа на экране, автоскролл

### Шаг 2.4 — Классы (JSON data)
- Скопировать `game/data/class_manager.h/.cpp` — загрузка данных классов из JSON
- Скопировать `game/combat/dice.h/.cpp` — броски кубиков
- Скопировать `game/data/experience_system.h/.cpp` — XP за уровень, HP gain, навыки
- Скопировать `game/data/skill_manager.h/.cpp` —.active/passive навыки классов
- Подключить JSON data files (classes.json, spells.json, abilities.json, level_table.json)
- **Результат**: 5 классов с уникальными статами, навыки по уровням

### Шаг 2.5 — MainMenu
- Скопировать `game/states/main_menu_state.h/.cpp` — главное меню
- Скопировать `game/states/settings_state.h/.cpp` — настройки (высота рендера, задержка повтора)
- Зарегистрировать состояния в `GameApp`: MainMenu → Settings → Play
- **Результат**: игра стартует с главного меню, есть "New Game", "Settings", "Back"

### Шаг 2.6 — ClassSelection
- Скопировать `game/states/class_selection_state.h/.cpp` — выбор класса
- Реализовать `CreateCharacterFromClass()` — создание персонажа из данных класса
- **Результат**: перед входом в PlayState выбираем класс, персонаж получает правильные статы

---

## Фаза 3: Монстры и боёвка

### Шаг 3.1 — Monster и MonsterManager
- Скопировать `game/combat/monster.h` — структура монстра (HP, AC, ATK, позиция, facing)
- Скопировать `game/combat/monster_manager.h/.cpp` — spawn/despawn/lookup
- Скопировать `game/data/monster_factory.h/.cpp` — создание монстров из JSON
- Скопировать `game/data/encounter_manager.h/.cpp` — таблица встреч по этажам
- **Результат**: монстры создаются по JSON-данным, 7 типов врагов

### Шаг 3.2 — MonsterRenderer (billboard)
- Скопировать `game/combat/monster_renderer.h/.cpp` — billboard-спрайты монстров
- Скопировать `game/combat/item_renderer.h/.cpp` — billboard-спрайты предметов на полу
- **Результат**: монстры видны как спрайты перед игроком, поворачиваются к камере

### Шаг 3.3 — CombatSystem
- Скопировать `game/combat/combat_system.h/.cpp` — разрешение атаки (d20 + bonus vs AC, криты)
- **Результат**: `PerformCombat()` бросает кубик, считает урон, криты

### Шаг 3.4 — TurnQueue
- Скопировать `game/combat/turn_queue.h` — очередь ходов (игрок → монстры)
- **Результат**: после хода игрока монстры атакуют по очереди

### Шаг 3.5 — CombatHandler
- Скопировать `game/combat/combat_handler.h/.cpp` — связка: атака, монстры, лог, спавн
- Добавить `SpawnDefault()` — спавн монстров на старте
- **Результат**: после Movement → TurnWaiting → монстры атакуют → TurnAnimating → Exploring

### Шаг 3.6 — Melee атака (Space)
- В `PlayState::processAttack()`: проверить монстра перед игроком → `PerformCombat()`
- Проверка AC, урон, лог, криты, контратака монстра
- **Результат**: Space атакует монстра перед собой, лог показывает результат

### Шаг 3.7 — GameOver (HP ≤ 0)
- Добавить `GameOver` state: popup "Your hero has fallen!", кнопки Restart / Back to Menu
- **Результат**: при 0 HP — popup, рестарт или выход в меню

---

## Фаза 4: Предметы и инвентарь

### Шаг 4.1 — Item и типы предметов
- Скопировать `game/combat/item.h` — `Item` с типами (оружие, броня, зелья, ключи, золото, еда)
- Скопировать `game/combat/inventory.h/.cpp` — инвентарь (макс. 32 предмета)
- Скопировать `game/combat/equipment.h/.cpp` — 8 слотов экипировки (оружие, щит, голова, тело, руки, ноги, кольцо, амулет)
- **Результат**: `Inventory` и `Equipment` работают, предметы добавляются/убираются

### Шаг 4.2 — ItemFactory
- Скопировать `game/data/item_factory.h/.cpp` — создание предметов из JSON
- Скопировать `game/data/item_stat_calculator.h/.cpp` — статы по редкости и материалу
- Скопировать `game/data/material_generator.h/.cpp` — генерация материалов
- Подключить JSON: items.json, weapons.json, armor.json, materials.json, prefixes.json, postfixes.json
- **Результат**: предметы создаются из JSON с правильными статами

### Шаг 4.3 — Предметы на полу
- Скопировать `game/combat/item_handler.h/.cpp` — спавн/подбор предметов
- `SpawnDefault()` — разбросать предметы по подземелью
- **Результат**: на полу видны предметы, Space подбирает ближайший

### Шаг 4.4 — Инвентарь UI (I)
- Реализовать в `PlayState::renderInventoryWindow()` — список предметов с тултипами
- Кнопки: Equip, Use, Drop
- **Результат**: I открывает инвентарь, можно экипировать/использовать/выкинуть

### Шаг 4.5 — Экипировка UI (E)
- Реализовать `renderEquipmentWindow()` — 8 слотов, кнопка Unequip, Equip All
- Показать суммарные статы экипировки
- **Результат**: E открывает окно экипировки

### Шаг 4.6 — Предметные дропы (монстры)
- При убийке монстра — шанс дропа предмета
- Золото за убийку
- **Результат**: монстры скидывают предметы и золото

---

## Фаза 5: RPG-системы

### Шаг 5.1 — XP и повышение уровня
- XP за убийство монстра (по таблице из JSON)
- Popup "Level Up!" — выбор STR/DEX/CON +2, +ATK, +HP
- **Результат**: после набора XP — level up, выбор статов

### Шаг 5.2 — Навыки и хотбар
- Скопировать систему `ActionSlot` — 9 слотов хотбара
- Окно навыков (K) — active/passive навыки, привязка к слотам
- **Результат**: K показывает навыки, можно назначить в слоты 1-9

### Шаг 5.3 — Заклинания (SpellSystem)
- Скопировать `game/combat/spell_system.h/.cpp` — каст заклинаний
- Скопировать `game/combat/status_effect.h/.cpp` — отравление, горение, заморозка и т.д.
- Скопировать `game/data/spell_manager.h/.cpp` — менеджер заклинаний из JSON
- Целинг: Space при нацеливании → ApplySpell → лог + анимация
- **Результат**: можно кастовать заклинания, наносить статус-эффекты монстрам

### Шаг 5.4 — Нацеливание (targeting)
- AoE targeting overlay — показ затронутых клеток (3x3, 5x5, beam, cross, line)
- Space подтверждает цель, Esc отменяет
- **Результат**: visual preview заклинаний перед кастом

### Шаг 5.5 — Заклинания UI (B)
- Spellbook window — классные заклинания + выученные
- Cast / Hotbar кнопки
- **Результат**: B открывает заклинания, можно кастовать или назначить на хотбар

---

## Фаза 6: Сохранение и загрузка

### Шаг 6.1 — Сериализация
- Скопировать `game/serialization.h/.cpp` — JSON-сериализация Character, Dungeon, MonsterManager, ItemDrops
- **Результат**: все игровые данные сериализуются в JSON

### Шаг 6.2 — SaveManager
- Скопировать `game/save_manager.h/.cpp` — SaveGame/LoadGame через `fopen`/`fread`
- **Результат**: можно сохранить и загрузить игру

### Шаг 6.3 — Save/Load UI (F5/F9)
- Окно выбора слотов (3 слота)
- Автосохранение при входе и смене этажа
- **Результат**: F5 сохраняет, F9 загружает, 3 слота

---

## Фаза 7: Подземелье — детали

### Шаг 7.1 — Лестницы и этажи
- NextFloor() / PrevFloor() — генерация нового этажа
- Space на лестницах — переход вниз/вверх
- **Результат**: можно спуститься на этаж ниже и вернуться обратно

### Шаг 7.2 — Двери
- Запертые двери: нужен ключ из инвентаря
- Открытые двери: проходимы
- **Результат**: двери блокируют путь, ключ открывает

### Шаг 7.3 — Ловушки
- При движении на ловушку — урон (x2 по этажу)
- Bararian — половина урона
- Search (F) — обезвреживание на текущей клетке
- Thief — автоматическое обнаружение в 3x3
- **Результат**: ловушки наносят урон, можно обезвредить

### Шаг 7.4 — Секретные стены
- Search (F) — ищет секретные стены в 3x3 → превращает в проходимые
- **Результат**: F находит секретные проходы

### Шаг 7.5 — Ресурсы и сбор
- Resource nodes — добываются Space с инструментом (pickaxe/fishing rod)
- Herb/flower/mushroom — без инструмента
- **Результат**: ресурсы добываются, дают материалы и XP

---

## Фаза 8: Голод и крафт

### Шаг 8.1 — Система голода
- `m_hunger` в Character,消費 по 1 за ход
- Предупреждение при 30, starving при 15, урон при 0
- Starving: -1 ATK, -1 AC
- **Результат**: голод дебаффит, можно умереть от голода

### Шаг 8.2 — Еда и баффы
- Еда восстанавливает голод, даёт временные баффы
- Сырое мясо — шанс отравления, варёное — "Well Fed"
- Суп — MP regen, Пища — ATK+2/AC+2
- Алкоголь — "Tipsy" → "Hangover"
- Испорченная еда — минимальное восстановление + яд
- **Результат**: еда разнообразна с механическим влиянием

### Шаг 8.3 — Крафт — рецепты
- Скопировать `game/crafting/crafting_recipe.h` — структура рецепта
- Скопировать `game/crafting/crafting_system.h/.cpp` — система крафта
- Загрузить рецепты и категории из JSON
- **Результат**: крафт-система с категориями (weaponsmith, armorsmith, alchemy, cooking)

### Шаг 8.4 — Крафт UI (C)
- Окно с табами категорий, список рецептов, кнопка Craft
- Проверка ингредиентов
- **Результат**: C открывает крафт, можно создавать предметы

### Шаг 8.5 — Операции ковки
- Создание оружия/брони из baseType + material
- Апгрейд (prefix), заточка (sharpness), ремонт (durability), postfix (эссенции)
- XP крафта (smithing, alchemy, cooking)
- **Результат**: полная ковочная система

---

## Фаза 9: Аудио и визуальные эффекты

### Шаг 9.1 — Звуки
- Скопировать `game/audio_manager.h/.cpp` — менеджер звуков
- Звуки: шаги, атака, заклинание, смерть монстров, подбор предметов
- **Результат**: при действиях играют звуки

### Шаг 9.2 — Музыка
- Фоновая музыка по локациям
- Настройки громкости (music/sfx volume)
- **Результат**: в dungeon играет музыка, громкость настраивается

### Шаг 9.3 — Анимации
- Скопировать `game/animation/animation_manager.h/.cpp` — billboard-эффекты
- Атака (swing), заклинание (fire/ice/lightning/holy), смерть (dissolve)
- **Результат**: визуальные эффекты при атаке, касте, смерти

### Шаг 9.4 — Damage flash
- Красная рамка при получении урона (таймер 0.3s)
- **Результат**: экран мигает красным при ударе

---

## Фаза 10: Overworld

### Шаг 10.1 — Overworld карта
- Скопировать `game/overworld/overworld.h` — cell-based карта с типами terrain
- Скопировать `game/overworld/overworld_cell.h` — типы клеток (grass, forest, water, mountain, road, town, dungeon entrance)
- **Результат**: overworld генерируется, 35x35 клеток

### Шаг 10.2 — Overworld рендер
- Скопировать `game/overworld/overworld_renderer.h/.cpp` — инстансированный рендер terrain
- Palette texture для типов terrain
- **Результат**: видна карта мира от первого лица

### Шаг 10.3 — OverworldState
- Скопировать `game/states/overworld_state.h/.cpp` — состояние overworld
- Сеточное движение по overworld
- **Результат**: после выхода из подземелья (лестница вверх на этаже 1) — попадаем в overworld

### Шаг 10.4 — Локации
- Скопировать `game/overworld/overworld_location.h` — города, храмы, лагеря, входы в подземелья
- Взаимодействие: Space на локации → popup
- **Результат**: на карте есть локации, можно взаимодействовать

### Шаг 10.5 — Города и торговля
- Скопировать `game/data/shop_manager.h/.cpp` — магазины
- Скопировать `game/data/npc_manager.h/.cpp` — NPC с диалогами
- Покупка/продажа предметов
- **Результат**: в городах можно торговать с NPC

### Шаг 10.6 — Случайные встречи
- При движении по wild terrain — шанс encounter → CombatEncounterState
- **Результат**: на overworld случаются случайные бои

### Шаг 10.7 — CombatEncounterState
- Скопировать `game/states/combat_encounter_state.h/.cpp` — арена для случайных встреч
- Small arena dungeon, spawn монстров, бой → награда → возврат в overworld
- **Результат**: случайные бои на отдельной арене

### Шаг 10.8 — Квесты
- Скопировать `game/data/quest_manager.h/.cpp` — загрузка квестов из JSON
- Журнал квестов (UI), отслеживание целей (убить N монстров, собрать предмет, поговорить с NPC)
- **Результат**: квесты принимаются у NPC, прогресс отслеживается

### Шаг 10.9 — День/ночь и время
- Цикл дня/ночи (0-23), освещение зависит от времени
- **Результат**: сутки сменяются, ночью темнее

### Шаг 10.10 — Быстрое перемещение
- Fast travel между открытыми локациями (за золото)
- **Результат**: можно телепортироваться между городами

### Шаг 10.11 — Автоповтор движения (overworld)
- Held repeat для удобства ходьбы по карте
- **Результат**: удержание клавиши = непрерывное движение

---

## Фаза 11: Полировка

### Шаг 11.1 — Debug инструменты
- Панель Turn System, Camera Debug, Dungeon Info
- **Результат**: F1 показывает полную отладочную информацию

### Шаг 11.2 — Опции (F2)
- Окно настроек: громкость, задержка повтора, высота рендера
- Сохранение в player_config.json
- **Результат**: настройки применяются и сохраняются

### Шаг 11.3 — HUD полировка
- Полоса HP (красная), MP (синяя), XP (фиолетовая), голод (оранжевая)
- Золото, уровень класса, баффы, статус-эффекты
- **Результат**: полный HUD с барами и статистиками

### Шаг 11.4 — Monster tooltip
- При взгляде на монстра: имя, HP bar, AC
- **Результат**: информативный тултип перед боем

### Шаг 11.5 — Cross-platform (WASM)
- Проверить `#ifdef __EMSCRIPTEN__` везде
- Убрать `thread/mutex`, `filesystem` из общего кода
- **Результат**: `build_web.bat` компилирует, игра работает в браузере

---

## Порядок зависимостей (краткая схема)

```
0.1 Logger → 0.3 Window → 0.5 Input → 0.6 Shader → 0.7 Texture → 0.8 Mesh → 0.9 Camera → 0.10 FBO → 0.11 Renderer → 0.12 StateMachine → 0.13 Resources → 0.14 ImGui → 0.15 GameApp
                                                                                      ↓
1.1 Chunk → 1.2 Dungeon → 1.3 DungeonRenderer → 1.4 PlayState → 1.5 Movement → 1.6 HUD → 1.7 Debug → 1.8 Map
                                                                                       ↓
2.1 Character → 2.2 TurnManager → 2.3 CombatLog → 2.4 Classes/Skills → 2.5 MainMenu → 2.6 ClassSelection
                                                                                       ↓
3.1 Monsters → 3.2 MonsterRenderer → 3.3 CombatSystem → 3.4 TurnQueue → 3.5 CombatHandler → 3.6 Melee → 3.7 GameOver
                                                                                       ↓
4.1 Item/Inventory → 4.2 ItemFactory → 4.3 ItemHandler → 4.4 Inventory UI → 4.5 Equipment UI → 4.6 Monster drops
                                                                                       ↓
5.1 XP/Level → 5.2 Skills/Hotbar → 5.3 Spells → 5.4 Targeting → 5.5 Spellbook
                                                                                       ↓
6.1 Serialization → 6.2 SaveManager → 6.3 Save/Load UI
                                                                                       ↓
7.1 Stairs → 7.2 Doors → 7.3 Traps → 7.4 Secrets → 7.5 Resources
                                                                                       ↓
8.1 Hunger → 8.2 Food/Buffs → 8.3 Crafting → 8.4 Crafting UI → 8.5 Smithing
                                                                                       ↓
9.1 Sounds → 9.2 Music → 9.3 Animations → 9.4 Damage flash
                                                                                       ↓
10.1 Overworld map → 10.2 Overworld render → 10.3 OverworldState → 10.4 Locations → 10.5 Shops/NPCs → 10.6 Random encounters → 10.7 CombatEncounter → 10.8 Quests → 10.9 Day/night → 10.10 Fast travel → 10.11 Auto-repeat
                                                                                       ↓
11.1 Debug → 11.2 Options → 11.3 HUD polish → 11.4 Monster tooltip → 11.5 WASM
```
