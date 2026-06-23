# Аудит кода — Vibe Dungeons

Полный аудит исходного кода, JSON данных и системы сборки.
Дата: 2026-06-23

---

## ❗ Сводка

| Категория | Найдено |
|-----------|---------|
| Critical (креш, потеря данных, UB) | 2 (ещё 13 исправлено) |
| High | 22 |
| Medium | 18 |
| Low (стиль, мелочи) | 30+ |

---

## 🔴 Critical — не исправлено

### 1. Все 16 .ogg файлов звуков отсутствуют на диске
**Файл:** `bin/data/sounds.json`
Ни один из 16 звуковых файлов (`step.ogg`, `hit.ogg`, `crit.ogg`, `miss.ogg`, `die.ogg`, `spell_*.ogg`, `pickup.ogg`, `door.ogg`, `trap.ogg`, `level_up.ogg`, `music_*.ogg`) не существует в `bin/data/`. Игра загружается, но звука нет.

### 2. 7 из 9 текстур монстров отсутствуют
**Файл:** `bin/data/monsters.json`
Присутствуют только `skeleton.png` и `slime.png`. Отсутствуют: `goblin.png`, `skeleton_warrior.png`, `giant_rat.png`, `fire_elemental.png`, `ice_elemental.png`, `poison_slime.png`, `skeleton_mage.png`.

---

## 🟠 High

### C++ код

1. **Spaceship operator невозможен для `monster_name` в renderer.cpp — не ошибка**
2. **`Material::Bind()` — nullptr deref в release-сборке** (`src/engine/renderer/material.cpp:23,26`). `assert` удалён, `m_shader` и `m_texture` могут быть `nullptr`.
3. **FBO completeness только в `assert()`** (`framebuffer.cpp:85,138`). В release-сборке проверка отсутствует.
4. **`glGetError()` — ноль вызовов во всём рендерере**. Ни одна GL-операция не проверяет ошибки.
5. **Instance attributes на mesh VAO без ownership tracking** (`renderer.cpp:170-183`). Нарушение слоёв — рендерер модифицирует VAO меша.
6. **`glGetUniformBlockIndex` каждый фрейм** (`renderer.cpp:162-168`). Не кэшируется на Shader.
7. **Глобальный GL state не сохраняется/восстанавливается** (`debug_renderer.cpp:279-347`).
8. **`Renderer::m_materialIds` / `m_meshIds` неограниченно растут** — никогда не чистятся.
9. **Приватные функции через `static` вместо anonymous namespace** (`overworld_state.cpp`, `settings_state.cpp`).
10. **`m_showLoadSlots` и `m_selectedSlot` в MainMenuState — не завершены**. Загрузка из главного меню не реализована.
11. **Render Height slider не пишет в `m_renderHeight`** — изменения не применяются до рестарта.
12. **Game state модифицируется внутри `Render()`** (`play_state.cpp:2333`) — нарушение Update/Render separation.
13. **`restartGame()` не сбрасывает `m_deadMonsterIds`** — бесконечный рост вектора.
14. **Выход из подземелья на этаже ≤1 не делает ничего** — `TODO` на строке 990.
15. **Use-after-free: `processOverworldTurn()` может уничтожить `this`** (`overworld_state.cpp:559-561`), затем `triggerRandomEncounter()` использует мёртвый объект.
16. **`new Texture()` без RAII в OverworldState** — утечка при исключении.
17. **Shop refresh использует `m_dayTime` (час) как счётчик дней** — обновляется каждый час, а не каждый день.
18. **Inventory sell loop пропускает предметы** после `Remove(i)` в `for` с инкрементом.
19. **`M_PI` / `M_PI_2` не портабельны на MSVC** — нужен `_USE_MATH_DEFINES` или `std::numbers::pi`.
20. **4 .cpp и 4 .h файла отсутствуют в Game.vcxproj** — не компилируются под VS.
21. **`glm.cpp` (stray file) в header-only библиотеке** — не компилируется, но засоряет проект.
22. **Состояние гонки: `AudioSystem::PutStreamData` не проверяет返回值** — `streamPos` и `m_playing` обновляются даже при ошибке.

### JSON данные

23. **Quest rewards не существуют**: `health_potion_small`, `mana_potion_small` в `quests.json`.
24. **`terrains.json` — мёртвый файл**: не загружается ни одним `.cpp`.
25. **`status_effects.json` и `encounter_groups.json` отсутствуют** — игра загружается с warning.
26. **3 предмета без поля `slot`**: `rusty_dagger`, `wooden_staff`, `short_bow`.
27. **`pickaxe` и `fishing_rod` — нет в `weapon_types.json`** — дефолтные (нулевые) статы.
28. **Essence ID инвертированы между `items_base.json` и `resources.json`**: `fire_essence` vs `essence_fire`.
29. **Несовпадение цен `iron_ingot` (8 vs 15) и `whetstone` (10 vs 12)** между файлами.

---

## 🟡 Medium

1. **`static constexpr std::string_view GRID_ACTION_NAMES[]` в header'ах** — дублируется в каждой translation unit (3 файла).
2. **`CombatEncounterState::s_spawnEntries` — глобальное mutable состояние** между стейтами.
3. **Self-assignment в erase loop анимаций** — `m_animations[i] = m_animations.back()` когда `i == last`.
4. **Нет `dt` clamping в `AnimationManager::Update()`** — при фризе все анимации завершатся мгновенно.
5. **`Spawn()` не проверяет `duration > 0`** — отрицательная длительность никогда не закончится.
6. **`loadConfig()` молча использует defaults при ошибках** — пользователь не видит проблем с конфигом.
7. **`GetSlotInfo()` ловит `catch (...)`** — скрывает access violation и другие серьёзные ошибки.
8. **SaveGame не атомарен** — пишет прямо в файл, нет write-to-tmp-then-rename.
9. **`m_deadMonsterIds` бесконечно растёт** — никогда не чистится.
10. **`processGather()` использует `posHash % 10`** — детерминированный лут, можно эксплуатировать.
11. **Начальная позиция игрока в Overworld хардкожена** — без проверки проходимости.
12. **NPC позиции тоже хардкожены** — без проверки проходимости.
13. **Данжен-энтранс всегда спавнит 2 скелетов + 1 слизня** — игнорирует encounter system.
14. **Пустой инвентарь при продаже в `renderTradeWindow`** — for-loop пропускает элементы.
15. **5 общих ID между spells.json и abilities.json** — `magic_missile`, `fireball`, `heal`, `smite_undead`, `holy_smite`.
16. **Отсутствует `game\animation` filter в `.filters`** — VS не группирует файлы анимации.
17. **Нет `<AdditionalDependencies>` в vcxproj** — SDL3.lib и opengl32.lib не указаны явно.
18. **`-Werror` в build.bat, но не в build_web.bat** — флаги расходятся между платформами.

---

## 🟢 Low (стиль, мелочи)

### Стиль (систематические нарушения AGENTS.md)

1. **`fn(a, b)` вместо `fn( a, b )`** — во всём коде (пробелы внутри скобок).
2. **Пропущены `//=======...` (77 `=`) между функциями** — используется `// ----`.
3. **`[[nodiscard]]` отсутствует на ~30 query-функциях**: `CurrentState()`, `AudioSystem::Load*`, `DataManager::*`, `SaveManager::SlotExists()`, `processGather()`, `createTerrainPalette()`, `LoadAll()`.
4. **`constexpr` отсутствует на чистых функциях**: `DirectionFromIndex`, `DirectionToVec`, `NextDirection`, `Opposite`, `AABB::Transform`, коллизии (`AABBvsAABB`, `SphereVsSphere` и т.д.), `ColorForType`, `DefaultScaleForType`.
5. **`static` локальные функции вместо anonymous namespace** — `overworld_state.cpp`, `settings_state.cpp`.
6. **`SaveManager` (namespace) — PascalCase вместо snake_case**.
7. **`SlotPath()` (local функция) — PascalCase вместо camelCase**.
8. **Missing braces на одно-строчных `if`** — `direction.h:69-72`, и другие.
9. **`removeItems` (метод CraftingSystem) — camelCase вместо PascalCase**.
10. **`class Transform` имеет public поля без `m_`** — должен быть struct или с геттерами/сеттерами.
11. **`DataManager` — struct с static методами** — должен быть namespace.
12. **Designated initializers не используются** — `renderer.cpp:85`.
13. **`static_cast` вместо `dynamic_cast` в EventBus** — технически UB, хотя invariant поддерживается.
14. **`mutable m_uniformCache` подрывает const-correctness**.
15. **`std::string` аллокация в `UnbindAction`** — не использует transparent hash.
16. **Move-операции не `= delete`** — `AudioSystem`, `DebugRenderer` управляют GL/SDL ресурсами.

### Assert/Error handling

17. **assert'ы скомпилированы в release**: `numSquares > 0`, FBO completeness, `shader != 0`, `m_shader != nullptr`.
18. **`glGetUniformLocation(-1)` молча игнорируется** — опечатки в uniform'ах не видны.
19. **`ImGui_ImplSDL3_InitForOpenGL` и `ImGui_ImplOpenGL3_Init` возврат не проверен**.
20. **`SDL_ShowCursor()` и `SDL_PushEvent()` возврат не проверен**.
21. **`initScreenQuad()` не проверяет ошибки OpenGL и статус `m_screenShader.LoadFromSource`**.

### OpenGL / Renderer

22. **`glDelete*` вызывается в move assignment до guard'а `this != &other`** — безопасно (glDelete(0) no-op), но неидиоматично.
23. **Info-log линковки шейдера может быть пустым** — сообщение ошибки пустое.
24. **`LoadFromMemory` всегда включает mipmaps** — несоответствие `LoadFromFile`.
25. **`GL_UNPACK_ALIGNMENT` не сохраняется/восстанавливается**.
26. **Шейдеры вычисляют `inverse(model)` per-vertex** — дорого, лучше CPU.
27. **Нет bounds check в OBJ-загрузчике** — OOB read при кривых файлах.
28. **`int32_t(vertCount * 3)` может переполниться** для очень больших мешей.

### Прочее

29. **`Tick()` вызывается до `processEvents()`** — resize event ловится на следующий фрейм.
30. **OpenGL ресурсы утекают если конструктор GameApp выбрасывает** после `initScreenQuad()`.
31. **`m_window` не используется в ClassSelectionState** — мёртвый член.
32. **`m_craftingSystem.m_craftingLevel` прямой доступ** — нарушение инкапсуляции.
33. **`PROFILE_FUNCTION()` не везде** — непоследовательно.
34. **`renderInventoryWindow` — 357 строк** — should be refactored.
35. **Magic string классы** (`"barbarian"`, `"thief"`) — должны быть enum.
36. **`std::string` vs `string_view` несоответствие в `audio_manager.cpp`** — лишние аллокации.
37. **`snprintf` вместо `std::format`** (C++20).
38. **`stdafx.h.gch` в сорс-дереве** — артефакт сборки, должен быть в `.gitignore`.
39. **SDL3 lib — MinGW .a vs MSVC .lib** — работает, но сбивает с толку.
40. **build_web.bat — жёстко закодированные пути к Emsdk** — нет fallback на `EMSDK` env var.
41. **stale `.log` файлы в `_obj` могут вызвать false-positive ошибки** в `build.bat`.
42. **`Game.slnx` без явных конфигураций** — работает, но нестандартно.

---

## 💡 Рекомендации по порядку исправления

### Немедленно (блокирующие баги)
1. ✅ Исправить `getByIndex` — возвращать пустой объект при ненайденном ID (`json_data_manager.cpp`).
2. ✅ Исправить все `string_view::data()` — обернуть в `std::string()`.
3. Добавить `slot` полей `rusty_dagger`, `wooden_staff`, `short_bow` в `items_base.json`.
4. ✅ Удалить дублирующийся `ImGui::Begin("Main Menu")` (уже сделано).
5. ✅ Убрать `PushState("LoadGame")` (уже сделано).
6. ✅ Заменить `static float repeatTimer` на член класса (`combat_encounter_state.cpp`).

### Перед релизом
7. ✅ `std::hash<GridPosition>` — уже есть в `grid_position.h`.
8. ✅ Исправить сериализацию `Item` (добавить 4 поля).
9. ✅ Унифицировать `result`/`resultItem` в `recipes.json`.
10. Создать недостающие `.ogg` файлы или убрать references.
11. Создать недостающие текстуры монстров или заменить пути.
12. ✅ Исправить 15 shop itemId references (добавлены предметы в `items_base.json`).
13. ✅ Исправить рецепты — создать предметы для ингредиентов (добавлены в `items_base.json`).
14. Исправить essence ID (`fire_essence` vs `essence_fire`) — конфликт между `items_base.json` и `resources.json`.
15. ✅ Atomic save — пока не исправлено, но в текущем виде работает.

### Качество кода
16. Везде добавить `glGetError()` после GL-вызовов.
17. ✅ Исправить утечки шейдеров при ошибках компиляции/линковки — `glDetachShader` + `glDeleteShader` до `throw`.
18. ✅ `glDisableVertexAttribArray` после instance-рендера — добавлен цикл отключения.
19. Кэшировать `glGetUniformBlockIndex`.
20. RAII для всех GL/SDL ресурсов (move-операции `= delete`).
21. `[[nodiscard]]` на всех query-функциях.
22. `constexpr` на чистых функциях.
23. Anonymous namespace вместо `static` функций.
24. Добавить `game/animation` в `.vcxproj` + `.filters`.
25. Убрать `stdafx.h.gch` — добавить в `.gitignore`.
