# Аудит кода

> Дата: 2026-06-17 (полный пересмотр)
> Состояние кодбазы: грид-based dungeon crawler с turn-based боем, предметами, инвентарём, JSON data layer, классовой системой (5 классов), XP/уровнями, системой умений (hotbar, skills window)
> Цель проекта: партийная пошаговая RPG в стиле Eye of Beholder (ретро)
> Сборка: g++ (MinGW) через `build.bat`, MSVC через `src/Game.slnx`, Emscripten через `build_web.bat`
> Строк кода (проект): ~9 500 (без 3rdparty)
> Выполнено: шаги 1–55 (см. FINISH.md)
> Фактический прогресс по ROADMAP: **Фаза 1–3 реализована полностью, Фазы 4+ не начаты**

---

## Критические проблемы

| # | Файл | Проблема |
|---|------|----------|
| C1 | ~~`game/states/play_state.cpp:873-924`~~ | ~~**Inventory loop bug**: после `Remove(i)` элементы сдвигаются влево, но `i++` пропускает следующий элемент~~ ✅ **FIXED** — изменён на `while` + `removed` flag |
| C2 | `game/combat/combat_system.cpp:7-14` | **ClassDamageBonus double-counting**: `GetDamageBonus()` добавляется дважды для Thief (`bonus` уже содержит +GetDamageBonus(), а caller снова прибавляет). Barbarian не получает `GetDamageBonus()` вообще |
| C3 | ~~`game/states/play_state.cpp:1970`~~ | ~~**applyRegen() дублирует RestoreMp**: прямой `SetMp()` вместо `RestoreMp(mpRegen)`~~ ✅ **FIXED** — логика вынесена в `TurnManager::ApplyRegen()`, использует `RestoreMp()` |
| C4 | `game/states/play_state.cpp:1521` | **ATK бонус при level-up**: хардкод `+1` для всех классов. Mage и War Priest должны получать меньше |
| C5 | `game/states/play_state.cpp:1522-1524` | **Level-up stat assignment**: сравнение по первому символу `'S'/'D'/'C'` вместо enum. Сломается при добавлении нового стата |
| C6 | `engine/audio_system.cpp:260-270` | **AudioSystem::Stop() не останавливает звук**: только удаляет запись из `m_playing`, данные в SDL-стриме доигрываются до конца |

---

## Крупные проблемы

| # | Файл | Проблема |
|---|------|----------|
| M1 | `game/states/play_state.cpp` (1449 строк) | **God-класс**: `PlayState` — 2073→1449 строк, 30+ полей, 20+ методов. Отвечает за рендер, инпут, аудио, UI (8+ окон), камеру, карту, level-up. **Извлечено**: `TurnManager`, `SaveManager`, `CombatHandler`, `ItemHandler`. Остаётся: `UIHandler` (inventory, map, options, hotbar, skills windows |
| M2 | `engine/audio_system.cpp` | **AudioSystem — мёртвый код**: `Play()` нигде не вызывается. Все 319 строк реализации (WAV/OGG, стриминг, каналы, громкость) — мёртвый груз. ROADMAP шаги 364-370 (звуки шагов, атак, музыки) не реализованы |
| M3 | (весь проект) | **Фазы 4-12 ROADMAP не начаты**: нет `EquipmentSlot`, `Equipment`, `ItemGenerator`, заклинаний, статус-эффектов, крафта, голода, overworld, NPC/торговли, квестов, звуков. Система генерации предметов (`ItemStatCalculator`) объявлена, но не вызывается нигде |
| M4 | `game/dungeon/dungeon.cpp` | **Только тестовая комната**: нет процедурной генерации (BSP), нет мульти-этажности, нет ловушек, секретов, ключей |
| M5 | `game/states/play_state.cpp:663-687` | **Turn система — фасад**: `processTurnWaiting()` сразу делает `Advance()` (no-op с 1 актором), нет блокировки хода. Игрок может двигаться и атаковать в одном "ходу" |
| M6 | `game/states/play_state.cpp:415-423` | **FixedUpdate() — no-op**: ничего не делает, хотя вызывается каждый кадр из `GameApp` |
| M7 | `engine/resource_manager.h` | **Raw pointer lifetime unsafety**: ресурсы возвращаются как сырые указатели. `Retain()`/`ReleaseResource()` нигде не вызываются в `play_state.cpp`. `CleanupUnused()` удалит ресурсы, которые всё ещё используются |
| M8 | `game/states/play_state.cpp:901-908` | **Mana potions и scrolls — заглушки**: предметы поднимаются, лежат в инвентаре, но использование выдаёт `"not yet implemented"` |
| M9 | `engine/audio_system.cpp:86,103` | **reinterpret_cast без гарантии alignment**: `uint8_t*` → `const float*` после `SDL_LoadWAV` может дать невыровненный доступ. На ARM — hardware fault |

---

## Проблемы средней тяжести

| # | Файл | Проблема |
|---|------|----------|
| N1 | `game/save_manager.cpp` | `SaveGame()`/`LoadGame()` — ручная fopen/fread/fwrite сериализация. Вынесена из PlayState в `SaveManager`, но по-прежнему не использует `JsonDataManager`. `"rb"` вместо `"wb"` для non-MSVC в `SaveGame()` |
| N2 | `game/combat/character.cpp:6-13` | `TakeDamage()` не учитывает AC/броню: весь урон проходит напрямую. Нет damage reduction |
| N3 | `game/combat/combat_system.cpp:99-163` | `UseAbility()` принимает `Monster*` nullable. Для melee/ranged типов нет null-check перед разыменованием (`defender->ac` в строке 131) |
| N4 | ~~`game/data/data_manager.h:19`~~ | ~~`Skill::cooldownRemaining` — runtime state в data-структуре~~ ✅ **FIXED** — поле удалено (было дублировано в `ActionSlot`) |
| N5 | `game/combat/inventory.h:22`, `game/ui/combat_log.h:25` | `size_t` в публичном API. AGENTS.md требует только `<cstdint>` типы |
| N6 | `game/ui/combat_log.cpp:9-10` | `erase()` с начала вектора — O(n) на каждую вставку после MAX_ENTRIES. Нужен кольцевой буфер |
| N7 | `engine/renderer/renderer.cpp:113-114` | `assert()` на null mesh/material — сработает только в debug. В release — падение |
| N8 | `game/states/play_state.cpp:85,1164` | `m_dungeon.GenerateTestRoom()` вызывается дважды (конструктор и Restart). Возможна утечка старой генерации |

---

## Мелкие проблемы

| # | Файл | Проблема |
|---|------|----------|
| P1 | `game/data/spell_manager.h`, `ability_manager.h`, `skill_manager.h` | **6 пустых файлов-алиасов**: `using X = DataManager;`. Никакой функциональности |
| P2 | `game/data/data_manager.cpp:79-87` | `GetSkill()` вызывает `GetAbilityData()` — нейминг путает skills и abilities |
| P3 | ~~`game/serialization.h:12`~~ | ~~`using json = nlohmann::json;` дублирует `core/json_alias.h:5`~~ ✅ **FIXED** — удалён дубликат |
| P4 | `game/combat/turn_queue.h` | TurnQueue — заглушка: всегда 1 актор, `Advance()` — no-op |
| P5 | `game/states/play_state.cpp:1826` | Вызов `UseAbility(..., nullptr)` для self/heal типов — UB если функция поменяется |
| P6 | `engine/renderer/shader.h:41` | `mutable std::unordered_map<std::string, int32_t>` для uniform cache — строковый поиск каждый кадр |
| P7 | `game/game_app.h:40` | `m_pendingCharacter` — хранится и зануляется после move. Внешние указатели инвалидируются |

---

## Соответствие AGENTS.md compliance

| Правило | Статус | Замечание |
|---------|--------|-----------|
| Precompiled headers (`stdafx.h` первым) | ✅ | |
| `#ifdef __EMSCRIPTEN__` платформозависимость | ✅ | |
| `final` на всех классах | ⚠ | `ShaderException` (shader.h:51) — не `final` |
| `[[nodiscard]]` на query-функциях | ⚠ | `GetPosition()` возвращает не-const ссылку |
| `constexpr` на константах | ✅ | |
| RAII для GL-ресурсов | ⚠ | Ручное управление, нет Retain() вызовов |
| Allman braces | ✅ | |
| `using` вместо `typedef` | ✅ | |
| `override` на переопределениях | ✅ | |
| C-style cast'ы | ⚠ | `reinterpret_cast` для GL offset'ов — допустимо |
| Designated initializers | ⚠ | Есть в profiler.cpp |
| Запрет `filesystem` | ✅ | |
| Tab-отступы | ✅ | |
| Public → protected → private | ❌ | camera.h: private/public/private/public — хаос |
| `#pragma once` | ✅ | |
| CamelCase для приватных методов | ✅ | |
| Пробел внутри скобок `fn( a, b )` | ❌ | **Не соблюдается по всему проекту** — везде `fn(a, b)` |
| Space around binary operators | ✅ | |
| `noexcept` на всех функциях | ⚠ | `SetName`, `SetClass` без `noexcept` |
| `size_t` не используется | ❌ | Inventory::Size(), CombatLog::MAX_ENTRIES |
| Non-const ссылки нарушают инкапсуляцию | ❌ | `GetPosition()` возвращает `GridPosition&` |
| `Trailing return type` запрещён | ✅ | |
| `auto fn() -> type` не используется | ⚠ | 0 случаев |

---

## Предложения по оптимизации

### Рендерер
- **Uniform cache**: заменить `std::unordered_map<std::string, int32_t>` на `std::array<int32_t, KNOWN_COUNT>` с enum-индексами
- **Frustum culling**: иерархический culling для чанков > 35×35
- **Release-safe asserts**: заменить `assert()` на `if (!ptr) continue;` в release-сборках

### Память
- **ResourceManager**: внедрить `std::shared_ptr` для автоматического lifetime management (уже есть TODO в коде)
- **Profiler**: добавить лимит записей (кольцевой буфер)
- **CombatLog**: заменить `std::vector` на `std::deque` или кольцевой буфер

### Аудио
- **OGG Load**: использовать `stb_vorbis_get_samples_short_interleaved()` с прямым буфером
- **Stop()**: действительно останавливать стрим (SDL_ClearQueuedAudio или SDL_PauseAudioDevice)
- **Play()**: начать вызывать для шагов, атак, заклинаний (ROADMAP шаг 366)

### Data layer
- **GetSkillsForClass()**: кэшировать после первого вызова
- **ItemStats**: добавить `elementalDamage` (отсутствует в ROADMAP шаг 74)
- **Save/Load**: переписать через `JsonDataManager::SaveToFile()`/`LoadFromFile()`

### Архитектура
- **PlayState**: разбить на `CombatHandler`, `ItemHandler`, `UIHandler`, `SaveManager`
- **TurnQueue**: или реализовать полноценно, или удалить как мёртвый код
- **Spell/Ability/Skill Manager'ы**: удалить пустые файлы-прокладки, использовать `DataManager` напрямую

---

## Сводка

### По цифрам
- **Критических багов**: 6 (Inventory loop ✅, double-counting, RestoreMp ✅, ATK, stat assignment, Stop())
- **Крупных проблем**: 9 (God-класс → 2073→1449 ✅, AudioSystem мёртв, Фазы 4-12 не начаты, dungeon заглушка, turn-система, FixedUpdate, ресурсы, potions/scrolls, alignment)
- **Проблем средней тяжести**: 8
- **Мелких проблем**: 7 (duplicate using json ✅, cooldownRemaining ✅)
- **Style violations**: 12+

### Рекомендации к немедленному исправлению
1. ✅ **CRIT-1**: `play_state.cpp` — исправлен (while + removed flag)
2. **CRIT-2**: `combat_system.cpp:14` — Barbarian не получает `GetDamageBonus()`
3. **CRIT-5**: `play_state.cpp` — ATK бонус должен зависеть от класса
4. **CRIT-6**: `audio_system.cpp:260` — вызывать `SDL_ClearQueuedAudio()` в `Stop()`

### Рекомендации к рефакторингу
1. **UIHandler** — извлечь из PlayState UI-рендеринг (inventory, map, options, hotbar, skills, level-up)
2. ✅ **PlayState** — разбит на `TurnManager`, `SaveManager`, `CombatHandler`, `ItemHandler` (2073→1449 строк)
3. **Save/Load** — переписать через JSON
4. **ResourceManager** — мигрировать на shared_ptr
5. **AudioSystem** — или начать использовать, или вырезать как мёртвый код
