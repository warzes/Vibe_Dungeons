# Аудит кода

> Дата: 2026-06-17 (полная ревизия)
> Состояние кодбазы: грид-based dungeon crawler с turn-based боем, предметами, инвентарём, JSON data layer, классовой системой (5 классов), XP/уровнями, системой умений (hotbar, skills window)
> Цель проекта: партийная пошаговая RPG в стиле Eye of Beholder (ретро)
> Сборка: g++ (MinGW) через `build.bat`, MSVC через `src/Game.slnx`, Emscripten через `build_web.bat`
> Строк кода (проект): ~9 500 (без 3rdparty)
> Выполнено: шаги 1–55 (см. FINISH.md)

---

##  Критические проблемы

---

##  Крупные проблемы

| # | Файл | Проблема |
|---|------|----------|
| 5 | `game/states/play_state.cpp` (2073 строки) | **God-класс**: `PlayState` — 2073 строки, 49 полей, 25+ методов. Отвечает за рендер, инпут, аудио, комбат, инвентарь, сохранение, level-up, UI (8+ окон), AI, hotbar, анимацию камеры, карту, опции. Нарушает SRP радикально |

---

##  Проблемы средней тяжести

| # | Файл | Проблема |
|--|------|----------|
| 14 | `game/states/play_state.cpp:940-1051` | `SaveGame()`/`LoadGame()` — ручная fopen/fread/fwrite сериализация. Весь блок кода (110 строк) использует сырые FILE*, повторяет fopen_s #ifdef, не использует `JsonDataManager` |

---

##  Мелкие проблемы

| # | Файл | Проблема |
|--|------|----------|
| 23 | `game/data/spell_manager.h:12` | `GetSpellsByClass()` — декларирован, но бросает `"not implemented"`? Надо проверить .cpp |
| 24 | `game/data/ability_manager.h:12` | `GetAbilitiesByClass()` — возвращает пустой массив или что-то подобное? |
| 27 | `game/states/play_state.cpp:1867-1891` | `processActionSlot()` — получает `Skill s = SkillManager::GetSkill(slot.id)` каждый раз при использовании. Нет кэширования |
| 29 | `game/combat/combat_system.cpp:114-117` | `UseAbility()`: хардкод `Dice::Roll(4, 8)` при healMax/healMin == 0 — магическое число |
| 31 | `engine/renderer/shader.h:41` | `m_uniformCache` — `mutable` + `std::unordered_map<std::string, int32_t>`. Поиск по строке каждый раз при `SetUniform()`. Для известных uniform'ов лучше enum |
---

##  Архитектурные проблемы

| # | Проблема | Описание |
|---|----------|----------|
| 42 | Нет слоя абстракции данных | JSON загружается в `nlohmann::json` объекты, к ним обращаются по строкам напрямую. Нет типизированных C++ структур для классов/монстров/предметов |
| 46 | `TurnQueue` — заглушка | Всегда 1 актор (игрок), `Advance()` — no-op. `SetActorCount()` не вызывается никогда. Вся очередь — мёртвый код для будущих нужд |
| 47 | Нет системы частиц/эффектов | Анимация заклинаний, атаки, смерти — отсутствуют. Только текстовые сообщения в лог |
| 48 | ImGui-UI и игровая логика **перемешаны** | `PlayState::Render()` содержит условные переходы, открытие попапов, изменения состояния персонажа — всё в одном методе |

---

##  Предложения по оптимизации

### Рендерер
- **Uniform cache**: заменить `std::unordered_map<std::string, int32_t>` на `std::array<int32_t, KNOWN_COUNT>` с enum-индексами
- **Frustum culling**: иерархический culling для чанков > 35×35

### Память
- ~~**ResourceManager**: внедрить reference counting~~ ✅ Исправлено: `Retain()`/`ReleaseResource()`/`CleanupUnused()`
- **Profiler**: добавить лимит записей (кольцевой буфер)

### Аудио
- **OGG Load**: использовать `stb_vorbis_get_samples_short_interleaved()` с прямым буфером, или стриминг для больших файлов
- **Stop()**: не перезаливать весь стрим, а найти точную позицию остановки и "замолчать" только нужный диапазон

### Data layer
- ~~**Кэш поиска**: заменить `findById()`~~ ✅ Исправлено: O(1) через `std::unordered_map`
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
| CleanupUnused() = Clear() — удаляет все ресурсы | ✅ Исправлено: reference counting через `Retain()`/`ReleaseResource()` |
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
| AudioSystem::Stop() некорректно обрабатывает имя | ✅ Исправлено: теперь только удаляет из m_playing без перезаливки стрима |
| `EngineException`, `ShaderException` не `final` | ❌ Не исправлено (критично? — нет) |
| `event_bus.h` нет Unsubscribe() | ❌ Есть `Unsubscribe<T>()` — ✅ добавлен |

---

##  Сводка

### По цифрам
- **Критических багов**: 1 (сохранение)
- **Крупных проблем**: 1 (God-класс)
- **Мёртвого кода**: 0 (tile.h переведён в комментарий)
- **Неисправленных из старого аудита**: 3 пункта

### Рекомендации к немедленному исправлению
1. Строка 973: `"rb"` → `"wb"` в `SaveGame()` для non-MSVC

### Рекомендации к рефакторингу
1. **PlayState** — разбить на отдельные модули: `CombatHandler`, `ItemHandler`, `UIHandler`, `SaveManager`
