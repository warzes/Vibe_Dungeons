# Аудит кода

> Дата: 2026-06-17 (полная ревизия)
> Состояние кодбазы: грид-based dungeon crawler с turn-based боем, предметами, инвентарём, JSON data layer, классовой системой (5 классов), XP/уровнями, системой умений (hotbar, skills window)
> Цель проекта: партийная пошаговая RPG в стиле Eye of Beholder (ретро)
> Сборка: g++ (MinGW) через `build.bat`, MSVC через `src/Game.slnx`, Emscripten через `build_web.bat`
> Строк кода (проект): ~9 500 (без 3rdparty)
> Выполнено: шаги 1–55 (см. FINISH.md)

---

##  Критические проблемы

| # | Файл | Проблема |
|---|------|----------|
| 1 | `game/states/play_state.cpp:973` | **Баг сохранения на non-MSVC**: `fopen(path, "rb")` вместо `"wb"` — на Linux/gcc `SaveGame()` открывает файл на **чтение**, а не на запись. Сохранение не работает |
| 2 | `engine/game_state.cpp:126-144` | `applyPush()`: если `OnEnter()` бросил исключение, `pop_back()` уничтожает новый state, но старый state уже получил `OnPause()` и не получает `OnResume()` — **разрыв контракта жизненного цикла**. Аналогично в `applyReplace()` |
| 3 | `game/states/play_state.cpp:1598-1638` | **Удвоение `m_character.hp = m_character.maxHp`**: в каждой ветке Level Up (STR/DEX/CON) строка записана **дважды подряд** (строки 1601 и 1604; 1617 и 1620; 1633 и 1636) — баг сброса HP на максимум, хотя могло быть другое намерение |
| 4 | `engine/audio_system.cpp:263-354` | `AudioSystem::Stop()` — **деструктивная перезапись** всего стрима: очищает и перезаливает все клипы заново. Если стрим уже был частично воспроизведён, вся синхронизация теряется. Клипы накладываются заново с нуля |

---

##  Крупные проблемы

| # | Файл | Проблема |
|---|------|----------|
| 5 | `game/states/play_state.cpp` (2073 строки) | **God-класс**: `PlayState` — 2073 строки, 49 полей, 25+ методов. Отвечает за рендер, инпут, аудио, комбат, инвентарь, сохранение, level-up, UI (8+ окон), AI, hotbar, анимацию камеры, карту, опции. Нарушает SRP радикально |
| 6 | `game/combat/character.h` (37 строк) | `Character` — публичная struct **без инкапсуляции**. Все поля (hp, mp, level, stats, inventory, actionSlots, learnedSpells) — публичные. Нет `const`-гарантий, нет методов валидации (hp не может быть > maxHp, mp не может быть < 0) |
| 7 | `game/states/class_selection_state.h:31` | `static Character s_pendingCharacter` — **глобальное состояние** для передачи данных между состояниями. При повторном входе может остаться старый мусор. Сломан при нескольких последовательных иницах |
| 8 | `game/combat/inventory.h` | `MAX_ITEMS = 32`, но хранилище — `std::vector<Item>` (динамический размер). Лимит проверяется только в `Add()`, но `Get()` по индексу работает независимо. Несоответствие контракта и реализации |
| 9 | `build.bat:145` | `set "OBJ_FILES="` / `for /R %%F in (*.o)` — **собирает .o файлы рекурсивно из всего `_obj`**, не фильтруя по изменённым. Если ранее была ошибка сборки, мусорные .o попадают в линковку |
| 10 | `engine/audio_system.cpp:145-148` | **Конверсия OGG S16→F32 в цикле**: поэлементное деление на 32768.0f — медленно для длинной музыки. Нет проверки `output == nullptr` после `stb_vorbis_decode_filename` |

---

##  Проблемы средней тяжести

| # | Файл | Проблема |
|---|------|----------|
| 11 | `game/combat/monster_manager.h` | `At(GridPosition)` — **линейный поиск** O(n) в векторе по всем монстрам. Для 2-5 монстров ок, но при 50+ на этаже — станет узким местом |
| 12 | `game/states/play_state.cpp:150-160, 1201-1214` | **Дублирование спавна монстров**: один и тот же код в `OnEnter()` и `RestartGame()`. Аналогично для дропа предметов |
| 13 | `engine/renderer/camera.h:130-131` | `m_animStartPosition`, `m_animStartYaw` — хранят состояние анимации, но в `updateFromYawPitch()` (стр. 60-80) **не вызывается `updateVectors()`** (которого вообще нет) — мёртвый код, упомянутый в старом аудите, не исправлен |
| 14 | `game/states/play_state.cpp:940-1051` | `SaveGame()`/`LoadGame()` — ручная fopen/fread/fwrite сериализация. Весь блок кода (110 строк) использует сырые FILE*, повторяет fopen_s #ifdef, не использует `JsonDataManager` |
| 15 | `game/states/play_state.cpp:1260-1358` | `renderOptionsWindow()` — **читает конфиг напрямую fopen каждую отрисовку кадра**, а не один раз при открытии окна. Потенциальная проблема производительности |
| 16 | `engine/renderer/renderer.cpp:96-106` | Сортировка `DrawCommand` по `getMaterialId`/`getMeshId` — теперь детерминированна, но **два прохода** по командам (сортировка + биндинг) + аллокация `m_instanceData`. Для малого числа команд — норма |
| 17 | `core/profiler.cpp:26-37` | `BeginSample()` — ищет сэмпл по имени (`std::ranges::find_if`) каждый раз. Для 20+ сэмплов в кадре — O(n) на каждый. Лучше `std::unordered_map` |
| 18 | `engine/resource_manager.cpp:150-154` | `CleanupUnused()` — **пустая** (no-op). Ранее была `Clear()`, теперь ничего не делает. Ресурсы никогда не выгружаются |
| 19 | `game/combat/inventory.cpp:4-12` | `Inventory::Add()` — при переполнении возвращает `false`, но **не говорит, какой именно предмет не поместился**. Вызывающий код просто считает инвентарь полным |
| 20 | `game/states/play_state.cpp:550-593, 610-663` | Два массива `ACTION_NAMES[]` дублированы в `processEdgeActions()` и `processHeldRepeat()` — идентичные списки из 6 имён |

---

##  Мелкие проблемы

| # | Файл | Проблема |
|---|------|----------|
| 21 | `game/states/main_menu_state.h:29-30` | `[[maybe_unused]]` на `m_window` и `m_input` — поля не используются, только хранятся. Лишний мусор в состоянии |
| 22 | `game/dungeon/tile.h` | `Tile`, `TileType` — **мёртвый код**: нигде не используется. В проекте используется `Cell` из `chunk.h` |
| 23 | `game/data/spell_manager.h:12` | `GetSpellsByClass()` — декларирован, но бросает `"not implemented"`? Надо проверить .cpp |
| 24 | `game/data/ability_manager.h:12` | `GetAbilitiesByClass()` — возвращает пустой массив или что-то подобное? |
| 25 | `game/serialization.h:86-96` | `to_json`/`from_json` для `ItemDrop` — определены в заголовке как `inline`, но используются только в `play_state.cpp`. Лучше в .cpp |
| 26 | `engine/audio_system.cpp:84-111` | `LoadWAV`: копирование сэмплов через `std::memcpy` при `SDL_AUDIO_F32` и через `SDL_ConvertAudioSamples` при S16. `SDL_ConvertAudioSamples` сам выделяет память, потом копируем — лишняя аллокация |
| 27 | `game/states/play_state.cpp:1867-1891` | `processActionSlot()` — получает `Skill s = SkillManager::GetSkill(slot.id)` каждый раз при использовании. Нет кэширования |
| 28 | `game/combat/combat_system.cpp:40-71` | `MeleeAttack()` — принимает `Character` **по const** &, но изменяет `defender` (Monster&). Хорошо, но тогда `ClassDamageBonus` и `ClassAtkBonus` должны быть `const` — и они есть. OK |
| 29 | `game/combat/combat_system.cpp:114-117` | `UseAbility()`: хардкод `Dice::Roll(4, 8)` при healMax/healMin == 0 — магическое число |
| 30 | `core/profiler.h:88-92` | Макросы `CONCAT`, `CONCAT2` — определены в хедере, могут конфликтовать с другими библиотеками |
| 31 | `engine/renderer/shader.h:41` | `m_uniformCache` — `mutable` + `std::unordered_map<std::string, int32_t>`. Поиск по строке каждый раз при `SetUniform()`. Для известных uniform'ов лучше enum |
| 32 | `core/json_data_manager.cpp:107-123` | `findById()` — линейный поиск по массиву JSON каждый запрос. Для 50+ записей — ок, для 500+ — медленно |
| 33 | `game/states/settings_state.h:8-17` | `PlayerConfig` — определение в `.h` для структуры, которая используется только в `settings_state.cpp`. Можно вынести в .cpp |

---

##  Дублирование кода

| # | Что дублируется | Где |
|---|-----------------|-----|
| 34 | Level-up логика (STR/DEX/CON) — три идентичных блока по 9 строк, различаются только именем стата | `play_state.cpp:1595-1641` |
| 35 | Спавн монстров (skeleton + slime) | `play_state.cpp:148-160` и `play_state.cpp:1201-1213` |
| 36 | Спавн предметов (heal potion + gold) | `play_state.cpp:226-239` и `play_state.cpp:1216-1228` |
| 37 | fopen_s / fopen #ifdef паттерн | Повторяется ~6+ раз: `save_game.cpp:967-973`, `load_game.cpp:997-1003`, `renderOptions:1263-1286, 1306-1326, 1340`, `json_data_manager:58-63` |
| 38 | Чтение JSON из файла (fseek/ftell/read/parse) | `json_data_manager.cpp:57-80` и `play_state.cpp:997-1017, 1272-1285` |
| 39 | ACTION_NAMES[] массив из 6 имён | `play_state.cpp:551-559` и `play_state.cpp:610-618` |
| 40 | `m_character.hp = m_character.maxHp` (дважды в каждой ветке) | `play_state.cpp:1601+1604, 1617+1620, 1633+1636` |
| 41 | `using json = nlohmann::json;` в глобальном namespace | `json_data_manager.h:3`, `skill_manager.h:8`, `serialization.h:12` — трижды |

---

##  Архитектурные проблемы

| # | Проблема | Описание |
|---|----------|----------|
| 42 | Нет слоя абстракции данных | JSON загружается в `nlohmann::json` объекты, к ним обращаются по строкам напрямую. Нет типизированных C++ структур для классов/монстров/предметов |
| 43 | `SkillManager`, `SpellManager`, `AbilityManager`, `ClassManager` — статические классы | Каждый со своей `static` имплементацией. Нет единого интерфейса для "data managers". Дублирование логики поиска |
| 44 | `GameStateMachine` не поддерживает стек более 2 уровней | `applyPush` вызывает `OnPause()` у текущего, но `applyPop` вызывает `OnResume()` только у оставшегося. При глубоком стеке (3+) — неверные колбэки |
| 45 | Отсутствие entity-ID | Монстры идентифицируются по позиции в векторе. При удалении монстра из середины вектора — все ссылки/указатели на него инвалидируются |
| 46 | `TurnQueue` — заглушка | Всегда 1 актор (игрок), `Advance()` — no-op. `SetActorCount()` не вызывается никогда. Вся очередь — мёртвый код для будущих нужд |
| 47 | Нет системы частиц/эффектов | Анимация заклинаний, атаки, смерти — отсутствуют. Только текстовые сообщения в лог |
| 48 | ImGui-UI и игровая логика **перемешаны** | `PlayState::Render()` содержит условные переходы, открытие попапов, изменения состояния персонажа — всё в одном методе |

---

##  Предложения по оптимизации

### Рендерер
- **Uniform cache**: заменить `std::unordered_map<std::string, int32_t>` на `std::array<int32_t, KNOWN_COUNT>` с enum-индексами
- **Сортировка команд**: перейти на `uint64_t sortKey = (materialId << 32) | meshId` для одной сортировки по ключу вместо парного сравнения
- **Frustum culling**: иерархический culling для чанков > 35×35

### Память
- **ResourceManager**: внедрить reference counting или `std::shared_ptr` для автоматической выгрузки
- **Profiler**: добавить лимит записей (кольцевой буфер)
- **MonsterManager::At()**: заменить на `std::unordered_map<GridPosition, Monster*, hash>` для O(1) доступа

### Аудио
- **OGG Load**: использовать `stb_vorbis_get_samples_short_interleaved()` с прямым буфером, или стриминг для больших файлов
- **Stop()**: не перезаливать весь стрим, а найти точную позицию остановки и "замолчать" только нужный диапазон

### Data layer
- **Кэш поиска**: заменить `findById()` (линейный поиск) на `std::unordered_map<std::string, size_t>` в `JsonDataManager`
- **SkillManager**: кэшировать `GetSkillsForClass()` после первого вызова

---

##  Соответствие AGENTS.md compliance

| Правило | Статус | Замечание |
|---------|--------|-----------|
| Precompiled headers (`stdafx.h` первым) | ✅ | |
| `#ifdef __EMSCRIPTEN__` платформозависимость | ✅ | |
| `final` на всех классах | ⚠ | `ShaderException` (shader.h:51) — не `final` |
| `[[nodiscard]]` на query-функциях | ✅ | |
| `constexpr` на константах | ✅ | |
| RAII для GL-ресурсов | ⚠ | `Texture::LoadFromFile` — `glDeleteTextures` перед `glGenTextures`. Ок, но ручное управление |
| Allman braces | ✅ | |
| `using` вместо `typedef` | ✅ | |
| `override` на переопределениях | ✅ | |
| C-style cast'ы | ⚠ | В `renderer.cpp:183` (`reinterpret_cast` для offset — ок, это под GL API) |
| Designated initializers | ⚠ | В `profiler.cpp:36` используется: `ProfileSample{ .name = ... }` — ок |
| Запрет `filesystem` | ✅ | Используется `fopen`/`fread` |
| Tab-отступы | ✅ | |
| Public → protected → private | ⚠ | `Camera` (camera.h): `private:` после public-секции, но внутри private есть ещё public-секция с сеттерами — смешение |
| `#pragma once` | ✅ | |
| CamelCase для приватных методов | ✅ | |
| Пробел внутри скобок `fn( a, b )` | ⚠ | Не везде соблюдается — часть вызовов с пробелами, часть без |
| Space around binary operators | ✅ | |

---

##  Неактуальные пункты из предыдущего аудита (удалены/исправлены)

| Было | Статус |
|------|--------|
| CleanupUnused() = Clear() — удаляет все ресурсы | ⚠ Исправлено: теперь no-op (но проблема не решена, ресурсы никогда не выгружаются) |
| Сортировка `DrawCommand` по указателям (ASLR) | ✅ Исправлено: теперь по числовым ID материалов/мешей |
| `m_orientation` в camera.h (updateVectors мёртвый код) | ⚠ Частично: `updateVectors()` не существует, но код ориентации переписан |
| `m_lastShader` — uint32_t vs GLuint | ✅ Формально корректно, оставлено как есть |
| `glBufferData` каждый кадр | ✅ Исправлено: теперь `glBufferSubData` при совпадении размера |
| `m_renderHeight` в конструкторе m_fbo | ✅ Оставлено как есть |
| `LoadWAV` конвертация S16→F32 | ✅ Оставлено как есть (SDL_ConvertAudioSamples используется) |
| `LoadFromFile`/`LoadFromMemory` дублирование | ✅ `applyTextureParams()` вынесена |
| `DeltaTime` пустой конструктор | ❌ Не исправлено: `DeltaTime::DeltaTime() noexcept {}` вместо `= default` |
| C-style cast в profiler.cpp | ✅ Больше нет |
| `std::vector<ProfileSample>` бесконечный рост | ❌ Не исправлено |
| AudioSystem::Stop() некорректно обрабатывает имя | ⚠ Частично: пытается чинить, но делает это деструктивно (см. Крит. проблема #4) |
| `EngineException`, `ShaderException` не `final` | ❌ Не исправлено (критично? — нет) |
| `event_bus.h` нет Unsubscribe() | ❌ Есть `Unsubscribe<T>()` — ✅ добавлен |

---

##  Сводка

### По цифрам
- **Критических багов**: 4 (сохранение, state machine, level-up, audio)
- **Крупных проблем**: 6 (God-класс, Character-struct, глобальное состояние, Inventory, сборка, OGG)
- **Дублирований**: 8 мест
- **Мёртвого кода**: 2 файла/структуры (tile.h, TileType)
- **Неисправленных из старого аудита**: 4 пункта

### Рекомендации к немедленному исправлению
1. Строка 973: `"rb"` → `"wb"` в `SaveGame()` для non-MSVC
2. Убрать дублирование `hp = maxHp` в level-up (3 места)
3. Вынести спавн монстров/предметов в методы `spawnDefaultMonsters()` и `spawnDefaultItems()`
4. Заменить `static Character s_pendingCharacter` на нормальный механизм передачи данных

### Рекомендации к рефакторингу
1. **PlayState** — разбить на отдельные модули: `CombatHandler`, `ItemHandler`, `UIHandler`, `SaveManager`
2. **Character** — инкапсулировать поля, добавить методы `TakeDamage()`, `Heal()`, `SpendMp()`, `IsAlive()`
3. **Data Managers** — объединить в единый кэширующий слой с типизированными структурами
4. **MonsterManager** — заменить вектор на хеш-таблицу по `GridPosition`
5. **AudioSystem::Stop()** — переписать без перезаливки всего стрима
