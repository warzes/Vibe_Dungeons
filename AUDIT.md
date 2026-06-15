# Аудит кода

> Дата: 2026-06-15 (актуализация)
> Состояние кодбазы: грид-based dungeon crawler с turn-based боем
> Цель проекта: партийная пошаговая RPG в стиле Eye of Beholder (ретро)
> Сборка: g++ (MinGW) через `build.bat`, MSVC через `src/Game.slnx`
> Строк кода (проект): ~5 500 (без 3rdparty)
> Выполнено: шаги 1–44 (см. FINISH.md)

---

##  Найденные проблемы

### Критические

| # | Файл | Проблема |
|---|------|----------|
| 2 | `core/event_bus.h` | **Нет `Unsubscribe()`** — AUDIT.md стр. 208 ошибочно утверждает обратное. Нельзя отписать конкретный колбэк, только полный `Clear()` |
| 3 | `src/game/states/settings_state.cpp` | Использует `std::ifstream`/`std::ofstream` — запрещено AGENTS.md для WASM. Нужен `fopen`/`fread` |
| 4 | `build.bat` | Нет флагов `-include src/stdafx.h` в единицах трансляции вне PCH-режима (работает только за счёт того, что каждая единица включает stdafx.h первой строкой — хрупко) |

### Средней тяжести

| # | Файл | Проблема |
|---|------|----------|
| 5 | `engine/audio_system.cpp:247-309` | `AudioSystem::Stop()` — сломанная логика: ищет первый попавшийся стрим вместо того, чтобы найти стрим для конкретного канала останавливаемого клипа. Перезаливает все оставшиеся клипы в один стрим, игнорируя разделение Music/SFX |
| 6 | `engine/renderer/renderer.cpp:72-80` | Сортировка `DrawCommand` по указателям (`reinterpret_cast<uintptr_t>`) — **недетерминированна** между запусками (ASLR). Нужна сортировка по ID материалов/мешей |
| 7 | `engine/renderer/camera.h:79` | `m_orientation` вычисляется в `updateFromYawPitch()` (стр. 88), но `updateVectors()` (стр. 93-98) **никогда не вызывается** — мёртвый код |
| 8 | `engine/resource_manager.cpp:150-155` | `CleanupUnused()` = `Clear()` — удаляет **все** ресурсы, а не только неиспользуемые. При смене состояний пересоздаёт шейдеры/текстуры/меши заново |
| 9 | `engine/renderer/renderer.cpp:120` | `m_lastShader` — тип `uint32_t`; `glGetUniformBlockIndex` возвращает `GLuint`, но значение `GL_INVALID_INDEX` (0xFFFFFFFF) корректно помещается — формально ок, но стилистически неконсистентно |
| 10 | `engine/renderer/renderer.cpp:114` | `glBufferData` с `GL_DYNAMIC_DRAW` каждый кадр — копируются все матрицы инстансинга, даже если ни одна не изменилась. Для кадров без новых команд был бы полезен `glBufferSubData` с флагом `GL_DYNAMIC_DRAW` при пересоздании буфера |

### Мелкие

| # | Файл | Проблема |
|---|------|----------|
| 11 | `game/game_app.cpp:157` | `m_renderHeight` — не-константное выражение в конструкторе `m_fbo` (зависит от `GetRenderHeightFromConfig()`). Корректно, но порядок инициализации хрупок |
| 12 | `engine/audio_system.cpp:77-108` | `LoadWAV`: потенциальное узкое место — конвертация S16→F32 в цикле. Лучше использовать SDL-шный `SDL_ConvertAudioSamples()` |
| 13 | `engine/renderer/texture.cpp:91-121` | `LoadFromFile/LoadFromMemory`: дублирование кода настройки текстурных параметров. Вынести в `applyParams()` |
| 14 | `core/profiler.cpp:26-27` | Есть `std::ranges::find_if`, но используются C-style касты для `m_sampleIndex[depth]` |
| 15 | `game/states/play_state.cpp` | Жёстко зашитые константы `NUM_MATERIALS=4`, `GRID=3`, `SPACING=2.5f` — не для production |
| 16 | `engine/delta_time.cpp:4` | Пустое тело конструктора — можно `= default` |
| 17 | `src/Game.vcxproj` | Нет `core/scoped_sdl.h` в списке ClInclude? Есть на строке 224. Ок. Но `engine/renderer/aabb.h` (стр. 230) — не существует |
| 18 | `build.bat` | Не включает `imgui_stdlib.cpp`, хотя vcxproj (стр. 139) — включает |

---

##  Предложения по оптимизации

### Рендерер
- **Instance buffer upload**: перейти на `glMapBufferRange` с `GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_BUFFER_BIT` для избежания копирования через CPU-буфер
- **Sort key**: ввести `uint64_t sortKey = (materialId << 32) | meshId` вместо сортировки по указателям
- **Frustum culling**: сейчас считается на CPU для каждого Submit. Для EoB- dungeon (до 1024 тайлов) достаточно, но для >5000 — нужен иерархический culling

### Память
- **Shader uniform cache**: `std::unordered_map<std::string, int32_t>` на каждый шейдер — поиск по строке. Заменить на `std::array< int32_t, KNOWN_UNIFORMS>` с enum-индексами для известных uniform'ов
- **ResourceManager**: хранить `std::weak_ptr<Resource>` или счётчик ссылок вместо полной очистки
- **Profiler**: `std::vector<ProfileSample>` растёт бесконечно. Добавить лимит или кольцевой буфер

### Аудио
- **Load**: кэшировать декодированные WAV/OGG чтобы не перезагружать при каждом `Play()`
- **Streaming**: для музыки >1 мин — потоковое декодирование, а не в память

### Сборка
- **PCH**: `build.bat` использует `-include src/stdafx.h` — хорошо, но замерять время сборки, т.к. GCH может быть медленнее на mingw
- **MSVC**: включить `/MP` (MultiProcessor Compilation) — уже есть (стр. 65 vcxproj: `MultiProcessorCompilation`)
- **Unity builds**: объединить мелкие .cpp в группы для ускорения (опционально)

---

##  Узкие места

### 1. ResourceManager::CleanupUnused
```cpp
void ResourceManager::CleanupUnused() noexcept
{
    Clear(); // <-- удаляет ВСЁ, даже нужное
}
```
Вызывается в `PlayState::OnExit()`. При возврате в MainMenu и повторном входе в Play все ресурсы пересоздаются. Для прототипа ок, но для production — нужен reference counting.

### 2. AudioSystem::Stop()
Некорректно обрабатывает остановку по имени — смешивает Music и SFX потоки. Если остановить SFX, может перезалить данные в Music-стрим.

### 3. Отсутствие тестов
Нет ни одного теста. Критичные для рефакторинга модули:
- `collision.h` (AABB, Sphere, Ray)
- `frustum.h` (extract + test)
- `transform.h` (иерархия + dirty)
- `event_bus.h` (подписка/публикация)
- `settings_state.cpp` (сериализация JSON туда-обратно)

### 4. WASM-сборка
- В `build.bat` нет цели для emscripten
- `std::ifstream/ofstream` в `settings_state.cpp` не скомпилируется под WASM
- Нет проверки, что шейдеры `#version 300 es` корректны в WebGL2

### 5. Отсутствие ассетов
100% графики — процедурная (checkerboard). Нет моделей, текстур, звуков. Для EoB-стиля нужны:
- Текстурный атлас dungeon-стен (16×16 пикселей в ретро-стиле)
- Спрайты монстров (billboard)
- UI-элементы (рамки, иконки, шрифты)
- OGG-файлы (ambient + SFX)

---

##  Развитие возможностей (под EoB)

### Приоритет: что добавить в движок сейчас

```cpp
// 1. Dungeon system (новый модуль src/game/dungeon/)
// Файлы:
//   - dungeon.h / .cpp        - загрузка/хранение/сериализация уровней
//   - tile.h                   - TileType, Tile, Level
//   - dungeon_renderer.h/.cpp  - генерация геометрии из тайлов + рендер
//   - visibility.h/.cpp        - raycasting на гриде (DDA)
//   - party.h / .cpp           - Party, Character, Inventory

// 2. Turn-based system (src/game/combat/)
//   - combat_state.h/.cpp       - конечный автомат боя
//   - action.h                  - ActionType (Move, Turn, Attack, Cast...)
//   - monster.h / .cpp          - Monster, AI (патруль/агр/преследование)
//   - dice.h                    - d20, d6 и т.д.

// 3. Retro UI (src/game/ui/)
//   - bitmap_font.h/.cpp       - битмап-шрифт из atlas (stb_truetype)
//   - hud.h/.cpp               - портреты, HP, AC, лог, мини-карта
//   - ui_renderer.h/.cpp       - ортографический рендер quad'ов с текстурами
```

### Интеграция с текущим рендерером
- DungeonRenderer **переиспользует** instancing из `Renderer::Submit()`:
  - Каждый тайл (Floor) = один instance с позиционной матрицей
  - Стены = отдельный batch с материалом стен
  - Потолок = отдельный batch
- `Camera` модифицировать в `GridCamera`:
  - Хранить `int32_t gridX, gridZ, floor, Direction facing`
  - `m_targetPosition`, `m_targetYaw` — lerp за 0.15-0.3s
  - Метод `SnapToGrid()` вызывать после анимации

### Сериализация
- Уже есть паттерн `to_json`/`from_json` в `settings_state.cpp`
- Расширить на `Dungeon`, `Party`, `Monster`, `Tile`
- Save/load через `fopen`/`fread` (WASM-safe) одной файловой функцией

### Рекомендации по ретро-рендеру (Eye of Beholder style)

1. **Разрешение**: рендер в FBO 320×240 (или 480×320) с `GL_NEAREST` фильтром — уже есть в `m_renderHeight` (settings)
2. **Шейдеры**: минимальные — текстурированные quad'ы с плоским освещением (no normal mapping)
3. **Биллборды**: монстры и предметы — textured quad с альфа-тестом (`discard` в фрагментном шейдере)
4. **Визуализация стен**: каждая стена — отдельный quad/plane без экструзии (плоские полигоны с текстурой камня), не full 3D BSP
5. **Пол/потолок**: один большой quad на клетку с тайловой текстурой, через instancing

### Структура кадра для EoB
```
1. Clear (FBO 320×240)
2. Render dungeon (стены → пол → потолок → монстры-биллборды) через Renderer::Submit()
3. Render overlay (HUD — портреты, HP, лог) в отдельный проход
4. Blit FBO на экран с nearest-neighbor (ретро-пикселизация)
5. ImGui (debug-меню)
```

---

##  Проверка связанных файлов

| Файл | Статус | Комментарий |
|------|--------|-------------|
| `README.MD` | ✅ Заполнен | Отражает текущее состояние (шаги 1–44) |
| `ROADMAP.MD` | ✅ Заполнен | Только будущие задачи (шаги 45+) |
| `FINISH.MD` | ✅ Создан | Всё выполненное (шаги 1–44 + отклонения) |
| `AUDIT.MD` | ✅ Обновлён | Текущий аудит |
| `TODO.md` | ❌ Не существует | Пока не нужен |
| `src/Game.vcxproj:230` | ✅ Исправлено | `engine/renderer/aabb.h` удалён |
| `core/event_bus.h` | ⚠ Нет `Unsubscribe()` | Добавить или исправить AUDIT.md |
| `build.bat` | ⚠ Нет emscripten-цели | todo |

---

##  Сводка по AGENTS.md compliance

| Правило | Статус |
|---------|--------|
| Precompiled headers (`stdafx.h` первым) | ✅ |
| `#ifdef __EMSCRIPTEN__` платформозависимость | ✅ |
| `final` на всех классах | ⚠ `EngineException`, `ShaderException` — не `final` (мелкие утилиты — ок) |
| `[[nodiscard]]` на query-функциях | ✅ |
| constexpr на константах | ✅ |
| RAII для GL-ресурсов | ✅ |
| Allman braces | ✅ |
| `using` вместо `typedef` | ✅ |
| `override` на переопределениях | ✅ |
| C-style cast'ы | ⚠ Есть в `profiler.cpp:31`, `debug_renderer.cpp:73`, `renderer.cpp:147` |
| Designated initializers | ⚠ `ProfileSample` push_back (profiler.cpp:36) — не использует designated initializers |
| Запрет `filesystem` | ⚠ `settings_state.cpp` использует `std::ifstream` |
| Tab-отступы | ✅ |
| Public → protected → private | ⚠ `Camera`, `Transform` — частичное нарушение |
