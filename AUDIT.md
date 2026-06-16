# Аудит кода

> Дата: 2026-06-16 (актуализация)
> Состояние кодбазы: грид-based dungeon crawler с turn-based боем, предметами, инвентарём, JSON data layer, классовой системой (5 классов), XP/уровнями, системой умений (hotbar, skills window)
> Цель проекта: партийная пошаговая RPG в стиле Eye of Beholder (ретро)
> Сборка: g++ (MinGW) через `build.bat`, MSVC через `src/Game.slnx`, Emscripten через `build_web.bat`
> Строк кода (проект): ~8 500 (без 3rdparty)
> Выполнено: шаги 1–55 (см. FINISH.md)

---

##  Найденные проблемы

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
| 16 | `engine/delta_time.cpp:4` | Пустое тело конструктора — можно `= default` |

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

### 5. Отсутствие ассетов
100% графики — процедурная (checkerboard). Нет моделей, текстур, звуков. Для EoB-стиля нужны:
- Текстурный атлас dungeon-стен (16×16 пикселей в ретро-стиле)
- Спрайты монстров (billboard)
- UI-элементы (рамки, иконки, шрифты)
- OGG-файлы (ambient + SFX)

---

##  Проверка связанных файлов

| Файл | Статус | Комментарий |
|------|--------|-------------|
| `README.MD` | ✅ Заполнен | Отражает текущее состояние (шаги 1–55) |
| `ROADMAP.MD` | ✅ Заполнен | Только будущие задачи (шаги 56+) |
| `FINISH.MD` | ✅ Создан | Всё выполненное (шаги 1–55 + отклонения) |
| `AUDIT.MD` | ✅ Обновлён | Текущий аудит |
| `TODO.md` | ❌ Не существует | Пока не нужен |
| `src/Game.vcxproj:230` | ✅ Исправлено | `engine/renderer/aabb.h` удалён |
| `core/event_bus.h` | ⚠ Нет `Unsubscribe()` | Добавить или исправить AUDIT.md |
| `build_web.bat` | ✅ Существует | Веб-сборка через Emscripten (заменяет emscripten-цель build.bat) |

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
