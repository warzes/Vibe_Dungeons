# Roadmap: Dungeon Crawlers — оставшиеся задачи

> **Выполнено (шаги 1–44)**: см. [FINISH.md](./FINISH.md)
> **Текущая работа**: Этап 1.6 (предметы и инвентарь)

---

##  Условные обозначения

- `[Engine]` — изменения в движке (renderer, core, engine/)
- `[Game]` — изменения в игровой логике (game/)
- `[Content]` — создание ассетов (текстуры, звуки, данные)
- `[Build]` — изменения сборки (build.bat, vcxproj)
- `[Test]` — написание тестов

---

##  Этап 1.6: Предметы и инвентарь (шаги 45–52)

### Шаг 45: `Item` и `ItemType`

```cpp
enum class ItemType : uint8_t
{
    Weapon, Armor, Shield, PotionHeal,
    PotionMana, Key, Gold, Scroll, QuestItem
};

struct Item final
{
    std::string name;
    ItemType type = ItemType::Gold;
    int32_t value = 0;    // heal amount, gold value, etc.
    int32_t bonus = 0;    // atk/ac bonus
};
```

### Шаг 46: `Inventory` — контейнер предметов

```cpp
class Inventory final
{
public:
    bool Add(const Item& item);
    bool Remove(size_t index);
    void Clear();
    [[nodiscard]] const Item* Get(size_t index) const noexcept;
    [[nodiscard]] size_t Size() const noexcept;
    static constexpr size_t MAX_ITEMS = 32;
private:
    std::vector<Item> m_items;
};
```

### Шаг 47: Интеграция инвентаря в Character

Добавить `Inventory inventory` в `Character`.
Добавить оборудование: `std::array<const Item*, 3> equipment{};` (weapon, armor, shield).

### Шаг 48: Предметы на полу

`ItemDrop` — структура на клетке подземелья.
Добавить `m_itemDrops : std::vector<ItemDrop>` в `PlayState`.

### Шаг 49: Подбирание предметов (G)

- Кнопка `G` (Action_Pickup)
- Проверить, есть ли ItemDrop на клетке игрока
- Если да — `inventory.Add(item)`, удалить дроп
- Сообщение в лог

### Шаг 50: Рендер предметов на полу

Биллборд-спрайт для каждого ItemDrop (как монстры, но другой цвет/форма).

### Шаг 51: Инвентарь UI (ImGui)

ImGui-окно "Inventory" (клавиша I):
- Список предметов
- Кнопка "Use" для PotionHeal (восстанавливает HP)
- Кнопка "Drop" (создаёт ItemDrop на полу)

### Шаг 52: Тест предметов

- Разместить тестовый ItemDrop (PotionHeal) на старте
- Подойти, нажать G — предмет в инвентаре
- Открыть инвентарь (I), использовать зелье — HP восстанавливается

---

##  Этап 1.7: Сохранение/загрузка прототипа (шаги 53–56)

### Шаг 53: Сериализация PlayerCharacter

JSON через nlohmann: `to_json`/`from_json` для Character.

### Шаг 54: Сериализация Dungeon (состояние)

Для каждого чанка: клетки с флагами, ItemDrop, Monster, двери.

### Шаг 55: Save/Load в PlayState

- F5 — `SaveGame("save.json")`
- F9 — `LoadGame("save.json")`
- `fopen`/`fwrite`/`fread` (WASM-safe)

### Шаг 56: Тест сохранения

Поиграть → сохранить → перезапустить → загрузить — всё на месте.

---

##  Этап 1.8: Полировка прототипа (шаги 57–60)

### Шаг 57: Ограничение видимости (FOV)

Классический roguelike FOV: только клетки перед игроком, ~5 клеток вперёд.

### Шаг 58: Туман войны

Невидимые клетки — не рендерить. Ранее виденные — тёмно-серые.

### Шаг 59: Карта (Tab)

ImGui-окно "Map": вид сверху, игрок — зелёная точка, монстры — красные.

### Шаг 60: Тест стабильности

15 минут без вылетов, все переходы режимов, монстры не в стенах.

---

##  Фаза 2: Полноценный dungeon crawler (шаги 61–120)

> Цель: Eye of Beholder-style party-based RPG с отрядом до 6 героев.

### Этап 2.1: От одного героя к партии (шаги 61–68)
- Party class, Formation, рендер партии, смена переднего ряда, экран создания персонажей, сериализация, партийный инвентарь, опыт.

### Этап 2.2: Продвинутая боевая система (шаги 69–77)
- Инициатива, множественные действия, AP, выбор цели, Monster AI (Patrol/Aggressive), группы монстров, боевой UI, анимация атаки, расширенный лог.

### Этап 2.3: Заклинания и магия (шаги 78–85)
- Spell, Spellbook, MP, реализация заклинаний, UI заклинаний, магические монстры, свитки, магические предметы.

### Этап 2.4: Подземелье — несколько этажей (шаги 86–93)
- Лестницы, генерация этажей, ключи и двери, секретные стены, ловушки, мини-карта, сброс состояния.

### Этап 2.5: Ретро UI (шаги 94–103)
- BitmapFont, ортографический проход, HUD с портретами, лог, карта, компас, отключение ImGui для game UI, пиксельные текстуры, EGA-палитра.

### Этап 2.6: Звук и музыка (шаги 104–108)
- Ambient, SFX (step, door, sword, hit, die, spell, pickup, trap), EventBus-привязка, AudioSystem настройка, MIDI-фон.

### Этап 2.7: Полировка и контент (шаги 109–120)
- Процедурная генерация (BSP), random encounter, торговец, квестовые предметы, триггеры, диалоги, scaling, stats экран, rest, автосохранение, балансировка, релиз.

---

##  Технический долг

- Проверять сборку под g++ и MSVC
- Добавлять unit-тесты для новой логики
- Профилировать через Profiler, FPS ≥ 60
- Emscripten-сборка для Web
- Asset pipeline: загрузка .png/.ogg из assets/
- Оптимизация: portalling, instancing стен
- Система сохранений с облачным слотом

---

##  Текущие настройки (player_config.json)

```json
{
  "gridMoveRepeatDelay": 0.1
}
```
