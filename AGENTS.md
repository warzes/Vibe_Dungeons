# Инструкции для агента

##  Система сборки
- Проект компилируется **gcc** (через `build.bat`) и **Visual Studio 2026** (через `src/Game.slnx`).
- **Web (WASM)**: сборка через `build_web.bat` (Emscripten) — **не использовать `build.bat`** для web.
- **Не пытаться** собрать через CMake, Ninja, или любые другие тулчейны.
- **Precompiled headers** (`stdafx.h`/`stdafx.cpp`) — всегда подключать `stdafx.h` первым в `.cpp` файлах.
- Исходный код находится в `src/`. После перемещения файлов обновляй пути в `build.bat`, `build_web.bat` и `src/Game.vcxproj`.

## Целевые платформы
- **Desktop (Windows/Linux)**: OpenGL 3.3 Core Profile, SDL3, gcc/MSVC.
- **Web**: WebGL2 (GLES3) через Emscripten, компиляция из C++ в WASM.

### Правила кросс-платформенности
- Весь код **должен** компилироваться под обе платформы без модификаций.
- Платформозависимые участки — через `#if defined(__EMSCRIPTEN__)` / `#else`.
- **Запрещены**:
  - `#include` платформозависимых заголовков (Windows.h, unistd.h и т.п.) в общих файлах.
  - Функции из `filesystem` (не работают в WASM) — использовать `fopen`/`fread` для ассетов.
  - `thread` / `mutex` (WASM single-threaded) — допустимы только под `#ifndef __EMSCRIPTEN__`.
  - Desktop-only GL-функции (GL 4.x, geometry/tesselation shaders) — только OpenGL 3.3 + WebGL2 subset.
- **OpenGL**: использовать только GL 3.3 core + WebGL2 общий подмножество. Все расширения проверять через `#ifdef` / glad.
- **Шейдеры**: `#version 330 core` (десктоп) / `#version 300 es` (WebGL2). Разные источники для каждой платформы через `#ifdef __EMSCRIPTEN__`.

## Язык и инструментарий
- **C++26** (`stdcpplatest`), **C23** (`stdclatest`).

## Требования к сторонним библиотека/зависимостям
- Доступные библиотеки расположены в папке `3rdparty`.
- Предпочитать однофайловые и header only библиотеки.
- Крупные 3rdparty подключаются централизованно через `stdafx.h`.

## Требования к коду
- **constexpr** на константах и утилитарных функциях.
- **RAII** для всех ресурсов.
- **`final`** на всех классах и структурах.
- **`[[nodiscard]]`** на всех фабричных функциях, геттерах и query-функциях.
- **C++20 Concepts** для шаблонных параметров (`requires`).
- **`std::span` и `std::string_view`** для параметров-диапазонов/строк вместо сырых указателей.
- **Кастомные целочисленные alias'ы не использовать** — только стандартные `<cstdint>` (`uint32_t`, `int32_t` и т.д.).
- **Designated initializers** для заполнения структур создания.

## Запрещено
- **ECS**: не использовать Entity-Component-System архитектуру.
- **Trailing return type** — не использовать (`auto fn() -> type`).
- **C++20 modules** — не использовать, только традиционные `.h`/`.cpp`.

## Стиль кода

### Именование
- Имена файлов: `snake_case` (например `gpu_buffer.h`, `sc_camera_node.cpp`).
- Классы и структуры: `PascalCase`.
- Публичные методы и глобальные функции: `PascalCase`.
- Приватные и локальные функции: `camelCase`.
- Макросы: `UPPER_SNAKE_CASE`.
- **Переменные-члены (private)**: префикс `m_`, `camelCase` — `m_cachedLocal`, `m_dirty`.
- **Глобальные переменные**: префикс `g_` — `g_scene`, `g_camera`.
- **Публичные поля (struct)**: без префикса.
- **Enum class values**: `PascalCase` (основной стиль), `UPPER_SNAKE_CASE` — только для bit-flag значений.
- **Константы**: `UPPER_SNAKE_CASE` — `MAX_LIGHTS`, `WHOLE_BUFFER`.

### Форматирование
- Только табуляция в отступах кода.
- **Allman style braces** — открывающая скобка на новой строке.
- **Пробел после ключевых слов**: `if (cond)`, `for (...)`, `while (cond)`, `switch (val)`.
- **Пробел после запятой**: `fn(a, b, c)`.
- **Пробел внутри скобок**: `fn( a, b )` не `fn(a, b)`.
- Space around binary operators: `a + b`, `flags & Mask`.
- **Конструкторы**: двоеточие на той же строке, каждый член с новой строки с ведущей запятой:

```cpp
SceneNode::SceneNode(std::string name_, NodeType type_)
	: name(std::move(name_))
	, type(type_)
{}
```

- **Разрешены однострочные инициализаторы** — если всё помещается.
- **Default member initializers** — предпочтительнее инициализаторов в списке для простых полей.
- **Указатели и ссылки**: звёздочка/амперсанд прилеплена к типу — `int* p`, `const glm::vec3& v`.
- **`const`** — всегда перед типом, "East const" запрещён.
- **Long conditional expressions** — перенос на операторах с отступом.
- **Switch** — `case` на уровне switch, тело case дополнительно с отступом.
- **Разделители**: `//=======...` (77 `=`) между функциями, `// ----` внутри функций для секций.

### Структура файла
- **Header guards**: только `#pragma once`.
- **Include order в .cpp**:
  1. `#include "stdafx.h"`
  2. Заголовок своего модуля (`#include "gpu_buffer.h"`)
  3. Остальные заголовки проекта (`#include "core_log.h"`)
  4. Standard library (`#include <cmath>`)
  5. 3rdparty (`#include <glm/glm.hpp>`)
- **Кавычки vs `<>`**: проект — `""`, stdlib/3rdparty — `<>`.

### Комментарии
- Предпочитать `//` однострочные.
- `/* */` — редко, только для многострочных блоков.
- Doxygen-стиль минимально, только `// @brief` если надо.
- `//<` для inline-документации полей структур.
- Разрешены комментарии на русском.

### Шаблоны
- `template<typename T>` — предпочтительно.
- `template<class T>` — допустимо для type-template параметров.
- **Constrains** — обязательны через `requires` для шаблонных параметров (C++20).

### Типы
- **Type aliases**: только `using X = Y;`, `typedef` запрещён.
- **Struct vs Class**: struct — для POD/данных/дескрипторов; class — для управляемых объектов с private-данными.
- **Visibility**: `public:` → `protected:` → `private:` (именно такой порядок).
- **Enum class** — всегда использовать вместо сырых `enum`.

### Современный C++
- **`= default` / `= delete`** для special member functions.
- **`override`** — обязателен при переопределении виртуальных методов.
- **`nullptr`** вместо `NULL` / `0`.
- **Move semantics**: `std::move()` при передаче владения.
- **C++20 Designated initializers** — для `CreateInfo` структур.
- **C++20 `std::span`, `std::string_view`, `std::optional`** — поощряются.
- **C++20 Ranges** — `std::ranges::find_if` и т.д.
- **Сырые массивы/указатели** — избегать, предпочитать `std::span`, `std::array`, `std::vector`.

### Отладка и проверки
- **`assert()`** из `<cassert>` для инвариантов и pre-condition.
- ошибки обрабатывать через исключения
- **Строка-пояснение** в assert: `assert(size % 4 == 0 && "Must be multiple of 4");`.

## Справочные файлы
- `ROADMAP.md` — **актуальный** поэтапный план. Только его использовать.
- `AUDIT.md` — аудит кода на исправление.
- `TODO.md` — разные задачи на будущее.

## Директории, которые не нужно читать
- `_archived/` — устаревшие/заменённые планы. Не читать, не учитывать в анализе.
