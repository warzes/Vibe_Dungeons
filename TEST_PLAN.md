# План тестирования игровых механик Vibe Dungeons

## 1. Существующая тестовая инфраструктура

- **Фреймворк**: кастомный минималистичный (`src/test/test_runner.h`) — макросы `TEST(name)`, `EXPECT_TRUE/FALSE/EQ/NEAR/STREQ`, авторегистрация, запись результатов в лог.
- **Запуск**: `tests` цель в `build.bat` (определена через `-DRUN_TESTS`).
- **Результат**: 80 тестов, все проходят.
- **Что покрыто**: только engine/core — AABB (5), Collision (15), Transform (9), EventBus (10), Exception (5), Camera (14), Frustum (6), DeltaTime (4), GameStateMachine (10).
- **Что НЕ покрыто**: вся game-логика (0 тестов).

## 2. Цель

Добавить тесты для **всех игровых механик** из `src/game/`. Тесты должны быть:
- Модульными (изолированными, без OpenGL/SDL/рендера)
- Быстрыми (миллисекунды на тест)
- Детерминированными (один и тот же seed → один и тот же результат)
- Кросс-платформенными (WASM-friendly)

## 3. Приоритеты

| Приоритет | Категория | Обоснование |
|-----------|-----------|-------------|
| **P0** | Core combat | Баланс боя — сердце игры. Ошибки здесь ломают всё. |
| **P1** | Character/Inventory/Equipment | Состояние героя сохраняется и влияет на всё. |
| **P1** | Status effects / Spells | Много перекрёстных взаимодействий. |
| **P2** | Items / Crafting | Большая поверхность, но менее критично. |
| **P2** | Dungeon generation | Детерминированная генерация — проверить инварианты. |
| **P3** | Overworld / Quests / NPC / Shops | Сложно мокать, много JSON-данных. |
| **P3** | Save/Load / Serialization | Интеграционные тесты. |

## 4. План тестов по модулям

### 4.0. Инфраструктура для тестов

**Файл**: `src/test/test_game_helpers.h` (новый)

Общие хелперы для всех game-тестов:
- `CreateTestCharacter(className)` — создаёт персонажа заданного класса с базовыми статами.
- `CreateTestMonster(typeId, level)` — создаёт монстра.
- `CreateTestItem(type, rarity)` — создаёт предмет с заданными параметрами.
- `MockDungeon(width, height)` — минимальная dungeon-сетка 5×5 без стен (или с заданным паттерном).
- `SeededRNG(seed)` — детерминированный генератор для тестов, где нужен random.

**Все игровые тесты** должны лежать в отдельных файлах `src/test/test_<module>.h`.

---

### 4.1. Dice (`src/game/combat/dice.h/.cpp`)

**Тесты**:
- `RollD20` — 1000 бросков, результат ∈ [1, 20], равномерное распределение (χ²-приближение).
- `RollD6` — результат ∈ [1, 6].
- `RollDice(2, 6)` — сумма 2d6 ∈ [2, 12].
- `RollDice(0, 6)` — пустой бросок = 0.
- `RollDice(1, 0)` — 0.
- Детерминированность при фиксированном seed (если используем `srand` — проверить порядок).

---

### 4.2. Character (`src/game/combat/character.h/.cpp`)

**Тесты**:
- `CreateCharacter` — базовые статы соответствуют классу (HP, MP, STR, DEX, CON, ATK, AC, damage range).
- `TakeDamage` — уменьшает HP, не уходит ниже 0.
- `Heal` — восстанавливает HP, не превышает maxHP.
- `SpendMp` — тратит MP, false если не хватает.
- `RestoreMp` — восстанавливает MP, не превышает maxMP.
- `MpRegenTick` — реген MP за тик (Mage +2, War Priest +1, остальные 0).
- `GetAc` — базовый AC + equip AC + hunger penalty + buff bonus.
- `GetAtkBonus` — базовая атака + equip + hunger + buffs.
- `GetDamageRange` — базовый урон + equip + sharpness.
- `EquipItem` — меняет статы (ATK, AC, damage).
- `UnequipItem` — возвращает статы обратно.
- `AddItem/RemoveItem` — инвентарь корректно обновляется.
- `AddItem` — полный инвентарь (32) возвращает false.
- `HasItem/FindItem` — поиск по типу/ID.
- `ApplyFoodBuff` — бафф применяется, статы меняются.
- `BuffTick` — тик уменьшает длительность, бафф снимается.
- `HungerTick` — голод уменьшается, при <15 применяется пенальти.
- `EatFood` — голод восстанавливается, бафф применяется.
- `SpoiledFood` — просроченная еда не применяется.
- `LevelUp` — статы растут (+STR/DEX/CON, +ATK, +HP).
- `LevelUpSkillGrant` — на нужном уровне даётся скилл.
- `AcceptQuest` — квест добавляется.
- `AdvanceObjective` — прогресс квеста увеличивается.
- `CompleteQuest` — квест завершается, награда выдаётся.
- `Reputation` — репутация меняет返回值 (взаимодействие с NPC).

---

### 4.3. CombatSystem (`src/game/combat/combat_system.h/.cpp`)

**Тесты**:
- `MeleeHit` — атака с высоким бонусом против низкого AC → hit.
- `MeleeMiss` — атака с низким бонусом против высокого AC → miss.
- `CriticalHit` — d20 = 20 → автопопадание + удвоенный урон.
- `CriticalMiss` — d20 = 1 → автопромах.
- `RangedHit` — дальняя атака с LOS → hit.
- `RangedNoLos` — дальняя атака без LOS → miss.
- `PointBlankPenalty` — атака в упор → -2 atk.
- `BackstabDamage` — Thief позади монстра → +2 dmg.
- `BackstabAtk` — Thief позади монстра → +2 atk.
- `ClassDamageBonus` — Barbarian +2, Thief +1 спереди / +2 сзади.
- `ClassAtkBonus` — Thief +1 спереди / +3 сзади, Barbarian +1 сзади.
- `ClassAcBonus` — Paladin +2, Thief +1, Mage -2.
- `AoE_3x3` — урон по области 3×3.
- `AoE_5x5` — урон по области 5×5.
- `AoE_Cross` — урон крестом.
- `AoE_Line` — урон линией.
- `AoE_Beam` — урон лучом (с LOS).
- `DexSaveHalfDamage` — сейв DEX уменьшает урон вдвое.
- `KillXpAward` — убийство даёт XP.
- `ResistanceCheck` — сопротивление элементу уменьшает урон.
- `ImmunityCheck` — иммунитет → 0 урона.
- `MonsterMeleeAttack` — монстр атакует персонажа.
- `MonsterRangedAttack` — монстр атакует издали (с проверкой LOS/distance).

---

### 4.4. CombatHandler (`src/game/combat/combat_handler.h/.cpp`)

**Тесты**:
- `ProcessMeleeAttack` — полный цикл: атака → урон → durability → смерть → XP.
- `WeaponDurabilityDegrade` — после атаки durability уменьшается.
- `WeaponBreak` — durability = 0 → оружие ломается.
- `ArmorDurabilityDegrade` — после получения урона броня теряет durability.
- `AmmoConsumption` — выстрел тратит стрелу/болт.
- `OutOfAmmo` — без амуниции флажок.
- `MonsterTurn` — полный ход монстра (выбор цели, атака, применение эффектов).
- `KillHandling` — убийство монстра → XP персонажу, удаление из менеджера.
- `DeathHandling` — смерть персонажа → GameOver.

---

### 4.5. TurnQueue (`src/game/combat/turn_queue.h`)

**Тесты**:
- `EmptyQueue` — пустая очередь не даёт следующего.
- `AddActor` — добавление actor'а.
- `NextActor` — циклический обход.
- `RemoveActor` — удаление из очереди.
- `RemoveCurrentActor` — удаление текущего, переход к следующему.
- `ResetQueue` — сброс с сохранением порядка.

---

### 4.6. MonsterManager (`src/game/combat/monster_manager.h/.cpp`)

**Тесты**:
- `SpawnMonster` — монстр создаётся и доступен по ID.
- `FindById` — поиск существующего/несуществующего.
- `DespawnMonster` — монстр удаляется.
- `RemoveDead` — все мёртвые монстры удаляются.
- `FindInFront` — поиск монстра перед персонажем (с учётом направления).
- `FindInLine` — поиск всех монстров на линии.
- `FindInRadius` — поиск в радиусе.
- `GroupAggro` — агр группы: триггер на одного → вся группа агрессивна.
- `IsGroupAggroed` — проверка состояния группы.
- `MonsterCount` — счётчик.
- `At` — получение по индексу с проверкой границ.

---

### 4.7. MonsterFactory (`src/game/data/monster_factory.h/.cpp`)

**Тесты**:
- `CreateMonsterByTypeId` — монстр создаётся с корректными статами из JSON.
- `CreateMonsterLevelScaling` — статы растут с уровнем (HP, AC, atk, damage).
- `CreateMonsterDefaultType` — неизвестный typeId → fallback.
- `BossMonsterFlags` — босс имеет isBoss = true, bossAbility.

---

### 4.8. SpellSystem (`src/game/combat/spell_system.h/.cpp`)

**Тесты**:
- `AcquireTarget` — цель в радиусе и LOS → успех.
- `AcquireTargetNoLos` — цель за стеной → неудача.
- `AcquireTargetOutOfRange` — цель слишком далеко → неудача.
- `CalculateDamage` — урон с учётом casting stat.
- `ApplyDamage` — урон применяется к цели.
- `AoESpellTargeting` — AoE (3×3, 5×5, Cross, Line, Beam) выбирает корректные цели.
- `AoESelfCentered` — AoE вокруг кастующего.
- `HealSpell` — лечение цели.
- `Cleanse` — снятие негативных эффектов.
- `StatusEffectApplication` — эффект накладывается через заклинание.
- `SpellBlockedByWall` — стена блокирует заклинание (LOS).
- `SpellClassRestriction` — заклинание с classReq не применяется другим классом.

---

### 4.9. StatusEffect (`src/game/combat/status_effect.h/.cpp`)

**Тесты**:
- `ApplyEffect` — эффект добавляется к цели.
- `ApplyEffectWithResistance` — сопротивление уменьшает/блокирует.
- `ApplyEffectWithImmunity` — иммунитет → эффект не накладывается.
- `ProcessTick_DoT` — тик наносит урон.
- `ProcessTick_Regen` — тик восстанавливает HP.
- `ProcessTick_Slow` — slow влияет на скорость.
- `ProcessTick_Stun` — stun блокирует действия.
- `ProcessTick_Blind` — blind влияет на atk.
- `ProcessTick_Shield` — shield поглощает урон.
- `ProcessTick_Invisibility` — невидимость даёт бонус к атаке.
- `ProcessTick_Silence` — silence блокирует заклинания.
- `AdvanceTurns` — счётчик тиков уменьшается, по 0 эффект снимается.
- `RemoveType` — удаление всех эффектов заданного типа.
- `RemoveAllNegative` — очистка всех негативных эффектов.
- `Cleanse` — снятие всех отрицательных эффектов.
- `DurationOverride` — переопределение длительности.
- `Stacking` — повторное наложение того же эффекта (обновление длительности?).
- `StatModifiers` — эффект меняет ATK/AC бонусы.
- `MultipleEffects` — несколько эффектов одновременно.
- `EmptyEffectList` — ProcessTick на пустом списке не падает.

---

### 4.10. Inventory (`src/game/combat/inventory.h/.cpp`)

**Тесты**:
- `AddItem` — предмет добавляется, возвращается слот.
- `AddItemFullInventory` — 32 предмета, 33-й не влезает.
- `RemoveItemBySlot` — удаление по слоту.
- `RemoveItemByInstanceId` — удаление по ID.
- `GetItem` — получение по слоту.
- `FindItemByType` — поиск первого предмета по типу.
- `CountItemByType` — подсчёт количества.
- `HasItem` — проверка наличия.
- `Clear` — очистка инвентаря.
- `SwapItems` — обмен местами двух предметов.
- `IterateItems` — проход по всем (непустым) слотам.

---

### 4.11. Equipment (`src/game/combat/equipment.h/.cpp`)

**Тесты**:
- `EquipItem` — предмет надевается в корректный слот.
- `EquipItemWrongSlot` — предмет не надевается в неверный слот.
- `UnequipItem` — предмет снимается.
- `UnequipEmptySlot` — пустой слот не вызывает ошибку.
- `GetEquippedItem` — получение по типу слота.
- `GetAllEquippedBonuses` — сумма всех бонусов от экипировки.
- `EquipAndReplace` — при надевании в занятый слот старый предмет возвращается.
- `ClearEquipment` — очистка всей экипировки.
- `SlotTypeMatching` — Weapon → WEAPON слот, Shield → SHIELD и т.д.

---

### 4.12. ItemFactory / ItemGenerator (`src/game/data/item_factory.h/.cpp`, `item_generator.h/.cpp`)

**Тесты**:
- `CreateBaseItem` — предмет создаётся с корректными базовыми полями.
- `CreatePotion` — зелье лечения/маны.
- `CreateGold` — золото с корректным количеством.
- `CreateScroll` — свиток с заклинанием.
- `GenerateWeapon` — процедурное оружие имеет статы (atk, damage, durability).
- `GenerateArmor` — процедурная броня имеет статы (ac, durability).
- `GenerateAccessory` — аксессуар имеет корректные бонусы.
- `RarityWeights` — распределение редкости (детерминированный seed).
- `ItemLevelScaling` — предмет высокого уровня имеет лучшие статы.
- `MaterialApplied` — материал влияет на статы и название.
- `PrefixApplied` — префикс влияет на статы.
- `PostfixApplied` — постфикс влияет на статы.
- `ItemStatCalculator` — расчёт финальных статов из base + material + prefix + postfix.
- `MaterialGenerator_NameAssembly` — сборка имени (префикс + материал + база + постфикс).
- `FoodCreation` — еда с корректными параметрами (nutrition, buff, expiration).
- `SpellScroll` — свиток с spellId и charges.

---

### 4.13. DungeonGenerator (`src/game/dungeon/dungeon_generator.h/.cpp`)

**Тесты**:
- `Generate_RoomBased` — сгенерированная карта содержит комнаты и коридоры, playerStart проходим.
- `Generate_RandomWalk` — связная карта, стартовая позиция проходима.
- `Generate_Cellular` — карта с пещерами, все регионы связны (или stairs есть).
- `Generate_Grid` — регулярная сетка комнат.
- `AllGenerators_StairsExist` — stairs up/down присутствуют на всех картах.
- `AllGenerators_NoOrphanCells` — все проходимые клетки достижимы из старта.
- `AllGenerators_PlayerStartValid` — playerStart — пол, не стена.
- `AllGenerators_FixedSeed` — один seed → одинаковая карта.
- `DungeonThemeApplied` — тема (Catacombs/Mine/Temple/...) влияет на типы клеток.
- `LockedDoorsGenerated` — двери генерируются (когда включены).
- `TrapsGenerated` — ловушки генерируются.
- `SecretWallsGenerated` — секретные стены генерируются.
- `ResourceNodesGenerated` — ресурсные узлы генерируются.

---

### 4.14. Dungeon (`src/game/dungeon/dungeon.h/.cpp`)

**Тесты**:
- `IsWalkable` — пол проходим, стена нет.
- `IsWalkable_OutOfBounds` — за пределами сетки = false.
- `HasLineOfSight` — прямая видимость без препятствий.
- `HasLineOfSight_Blocked` — стена блокирует LOS.
- `HasLineOfSight_Door` — открытая дверь не блокирует, закрытая блокирует.
- `GetLineCells` — все клетки на линии между A и B.
- `GetLineCells_Empty` — A == B → одна клетка.
- `StairsUpDown` — лестницы корректно соединяют уровни.
- `MonsterOccupancy` — занятость клетки монстром.
- `SetMonsterOccupancy` — установка/очистка занятости.
- `CellProperties` — чтение/запись свойств клетки.
- `DungeonResize` — смена размера сетки.

---

### 4.15. GridPosition / Direction (`src/game/grid_position.h`, `direction.h`)

**Тесты**:
- `GridPositionAdd` — сложение позиций.
- `GridPositionDirectionOffset` — DirectionToOffset (North = (0,-1) и т.д.).
- `DirectionRotateLeft/Right` — поворот направления.
- `DirectionOpposite` — противоположное направление.
- `DirectionToString` — строковое представление.
- `GridPositionEquality` — сравнение.
- `GridPositionHash` — хеш для использования в unordered_map.

---

### 4.16. TurnManager (`src/game/turn_manager.h/.cpp`)

**Тесты**:
- `ModeTransition` — Exploring → TurnWaiting → CombatTurn → TurnAnimating → Exploring.
- `StartCombat` — переход в боевой режим, инициализация очереди.
- `EndCombat` — выход из боя, возврат в Exploring.
- `PlayerActionInExploring` — действие в Exploring запускает TurnAnimating.
- `PlayerActionInCombat` — действие в CombatTurn запускает обработку.
- `EnemyTurn` — после игрока ход переходит монстрам.
- `TurnCountIncrement` — счётчик ходов растёт.

---

### 4.17. EncounterManager (`src/game/data/encounter_manager.h/.cpp`)

**Тесты**:
- `GetEncounterForFloor` — случайная встреча соответствует этажу.
- `EncounterWeightedSelection` — взвешенный выбор (много итераций, стат. проверка).
- `GroupAggroConfig` — группа корректно настраивается.
- `MinMaxFloorFilter` — встреча не выпадает вне диапазона этажей.
- `EmptyFloor` — на этаже без встреч возвращается пусто.

---

### 4.18. ExperienceSystem (`src/game/data/experience_system.h/.cpp`)

**Тесты**:
- `XpThreshold` — XP для каждого уровня из level_table.json.
- `CalculateLevel` — уровень по XP.
- `AwardKillXp` — XP за убийство монстра = monster.xpReward.
- `LevelUpEvent` — при достижении порога levelUp = true.
- `MaxLevel` — 20 уровень не даёт дальнейшего повышения.
- `SkillGrantsOnLevel` — классовые скиллы выдаются на нужных уровнях.

---

### 4.19. CraftingSystem (`src/game/crafting/crafting_system.h/.cpp`)

**Тесты**:
- `GetRecipesByCategory` — рецепты категории (weaponsmith/armorsmith/alchemy/cooking).
- `CanCraft` — достаточно ингредиентов и уровня.
- `CanCraft_MissingIngredients` — не хватает → false.
- `CanCraft_LowLevel` — низкий уровень навыка → false.
- `CraftItem` — крафт消耗 рецепт → предмет.
- `CraftItem_IngredientsConsumed` — ингредиенты списаны.
- `CraftItem_XpGained` — опыт крафта увеличивается.
- `CraftingLevelUp` — уровень крафта повышается.
- `SharpenWeapon` — заточка увеличивает sharpness.
- `RepairItem` — ремонт восстанавливает durability.
- `AddPostfix` — добавление постфикса к предмету.
- `UpgradeItem` — апгрейд предмета.
- `EnchantItem` — зачарование брони.
- `DiscoverRecipe` — открытие нового рецепта.
- `RecipeLockedByLevel` — рецепт не виден, пока не достигнут уровень.

---

### 4.20. Overworld (`src/game/overworld/overworld.h/.cpp`)

**Тесты**:
- `GridDimensions` — размер 64×64.
- `TerrainAt` — получение типа местности.
- `Walkability` — Grassland/Road → true, Water/Mountain → false.
- `MoveCost` — Grassland = 1, Forest = 2, Mountain = ∞.
- `EncounterChance` — разный шанс для разной местности.
- `FogOfWar` — непосещённые клетки скрыты.
- `VisitCell` — посещение открывает fog of war.
- `VisibilityRadius` — видимые клетки в радиусе с учётом LOS.
- `LocationAt` — получение локации в клетке.
- `AddLocation` — добавление/удаление локации.
- `FastTravel` — телепорт к посещённой локации (если есть gold).

---

### 4.21. QuestManager (`src/game/data/quest_manager.h/.cpp`)

**Тесты**:
- `GetQuestById` — загрузка квеста из JSON.
- `QuestObjectives` — корректные типы целей (Kill/Collect/Explore/Talk).
- `QuestRewards` — XP, gold, items выдаются.
- `QuestChain` — nextQuestId ведёт к следующему квесту.
- `QuestStatusFlow` — Inactive → Active → Completed.
- `QuestPrerequisites` — квест не активируется без выполнения предшествующего.
- `QuestProgressTracking` — счётчик выполнения цели.

---

### 4.22. Save/Load (`src/game/save_manager.h/.cpp`, `serialization.h`)

**Тесты** (интеграционные):
- `SaveAndLoadCharacter` — сохранённый персонаж восстанавливается с теми же статами.
- `SaveAndLoadInventory` — все предметы в инвентаре сохраняются.
- `SaveAndLoadEquipment` — все экипированные предметы сохраняются.
- `SaveAndLoadDungeon` — сетка подземелья восстанавливается.
- `SaveAndLoadMonsters` — монстры и их состояния.
- `SaveAndLoadCombatLog` — лог сохраняется/загружается.
- `SaveSlotInfo` — информация о слоте корректна.
- `LoadCorruptedSave` — повреждённый файл → graceful error.
- `EmptySlotInfo` — пустой слот возвращает корректные данные.

---

### 4.23. Action / ActionSlot (`src/game/combat/action.h`)

**Тесты**:
- `ActionTypeEnum` — все типы действий enum корректны.
- `ActionSlotCooldown` — кулдаун отслеживается.
- `ActionSlotReady` — готовность после кулдауна.
- `ActionSlotType` — ability/spell/item distinction.
- `ActionSlotUse` — использование action consume cooldown.

---

### 4.24. DataManager / JsonDataManager (`src/core/data_manager.h/.cpp`, `json_data_manager.h/.cpp`)

**Тесты**:
- `LoadAllJsonFiles` — все JSON файлы загружаются без ошибок парсинга.
- `GetClassDefinition` — класс из classes.json загружается корректно.
- `GetSpellDefinition` — заклинание из spells.json.
- `GetAbilityDefinition` — абилка из abilities.json.
- `GetSkillDefinition` — скилл.
- `GetMonsterDefinition` — монстр из monsters.json.
- `GetLevelTable` — level_table.json.
- `ItemStatCalculation` — статы предмета считаются по JSON-правилам.
- `MissingKey` — запрос несуществующего ключа → fallback/default.
- `JsonSchemaValidation` — все JSON файлы соответствуют ожидаемой схеме.

---

### 4.25. AnimationManager (`src/game/animation/animation_manager.h/.cpp`)

**Тесты**:
- `SpawnEffect` — эффект создаётся.
- `UpdateTick` — тик обновления уменьшает время жизни.
- `RemoveExpired` — истёкшие эффекты удаляются.
- `GetActiveEffects` — список активных эффектов.
- `ClearAll` — очистка всех эффектов.

---

### 4.26. CombatLog (`src/game/ui/combat_log.h/.cpp`)

**Тесты**:
- `AddEntry` — запись добавляется.
- `MaxEntries` — при 128 записях старые удаляются.
- `Clear` — очистка лога.
- `GetEntries` — получение записей (range).
- `ColorCoding` — тип записи соответствует цвету.

---

### 4.27. NPC / Shop (`src/game/data/npc_manager.h/.cpp`, `shop_manager.h/.cpp`)

**Тесты**:
- `GetNpcById` — загрузка NPC из JSON.
- `NpcDialogue` — текст диалога.
- `NpcTypeClassification` — Merchant/Trainer/QuestGiver/Barkeeper.
- `NpcShopTable` — shopId → таблица товаров.
- `GenerateShopInventory` — генерация списка товаров по таблице.
- `ShopItemWeightedSelection` — взвешенный выбор товаров.
- `BuyItem` — покупка списывает золото.
- `SellItem` — продажа даёт золото.
- `MaxShopItems` — не больше максимума.

---

### 4.28. EventBus (расширение — `src/core/event_bus.h`)

**Тесты** (уже 10 тестов, можно добавить):
- `GameEvents` — подписка на игровые события (OnCombatStart, OnKill, OnLevelUp, OnItemPickup).
- `MultipleEventTypes` — несколько типов событий на одной шине.

---

### 4.29. Интеграционные тесты (сценарии)

**Файл**: `src/test/test_scenarios.h`

Полные игровые сценарии без рендера:

- `FullCombatCycle` — персонаж → встреча с монстром → бой → победа/поражение.
- `LevelUpCycle` — убийство монстров → XP → level up → статы растут.
- `EquipmentCycle` — подбор предмета → экипировка → статы меняются → снятие → статы возвращаются.
- `HungerDeath` — голод → starvation → penalties → еда → восстановление.
- `ItemDegradation` — оружие ломается после N ударов.
- `QuestCompletionCycle` — получение квеста → убийство целей → завершение → награда.
- `OverworldTravelAndDungeon` — перемещение по миру → вход в подземелье → исследование.
- `SaveLoadGameCycle` — сохранение → загрузка → состояние идентично.
- `SpellAndStatusEffectCycle` — каст заклинания → эффект → тики → снятие.
- `CraftingCycle` — сбор ресурсов → крафт → использование предмета.
- `ShopTradeCycle` — покупка → продажа → золото корректно меняется.
- `GroupAggroCombat` — атака на одного монстра → вся группа агрится.
- `AoeFriendlyFire` — AoE заклинание не бьёт по своим (если так задумано).
- `FoodSpoilageAndBuff` — еда портится → не применяется; свежая еда → бафф.
- `MultipleStatusEffects` — DoT + Slow + Stun одновременно на одной цели.

---

## 5. Методология

### 5.1. Arrange-Act-Assert (AAA)

Каждый тест следует паттерну:

```cpp
TEST(Character_TakeDamage)
{
	// Arrange
	auto hero = CreateTestCharacter("Barbarian");
	int initialHp = hero.GetHp();

	// Act
	hero.TakeDamage(10);

	// Assert
	EXPECT_EQ(hero.GetHp(), initialHp - 10);
}
```

### 5.2. Dependencies

- **Все зависимости (Dungeon, MonsterManager) — замоканы в юнит-тестах.**
- Для модульных тестов: `Character` не требует `MonsterManager`, `Dungeon` не требует `Renderer`.
- Интеграционные тесты используют реальные классы без рендера.

### 5.3. JSON-данные

Тесты DataManager и фабрик **загружают реальные JSON-файлы** из `bin/data/`. Это гарантирует, что данные корректны.

### 5.4. Seeds

Все тесты, использующие random, принимают seed как параметр. По умолчанию `seed = 42` для детерминированности.

### 5.5. Покрытие

Цель: **>200 тестов** после реализации плана (сейчас 80).

---

## 6. Оценка трудозатрат

| Модуль | Тестов | Сложность | Файлов |
|--------|--------|-----------|--------|
| Dice | 5 | Низкая | 1 |
| Character | 25 | Средняя | 1 |
| CombatSystem | 20 | Высокая | 1 |
| CombatHandler | 8 | Высокая | 1 |
| TurnQueue | 6 | Низкая | 1 |
| MonsterManager | 10 | Средняя | 1 |
| MonsterFactory | 4 | Низкая | 1 |
| SpellSystem | 12 | Высокая | 1 |
| StatusEffect | 20 | Средняя | 1 |
| Inventory | 11 | Низкая | 1 |
| Equipment | 9 | Низкая | 1 |
| ItemFactory/Generator | 14 | Средняя | 2 |
| DungeonGenerator | 13 | Средняя | 1 |
| Dungeon | 12 | Средняя | 1 |
| GridPosition/Direction | 7 | Низкая | 1 |
| TurnManager | 7 | Средняя | 1 |
| EncounterManager | 5 | Низкая | 1 |
| ExperienceSystem | 6 | Низкая | 1 |
| CraftingSystem | 15 | Средняя | 1 |
| Overworld | 13 | Средняя | 1 |
| QuestManager | 7 | Средняя | 1 |
| Save/Load | 9 | Высокая | 2 |
| Action | 5 | Низкая | 1 |
| DataManager | 10 | Средняя | 2 |
| AnimationManager | 5 | Низкая | 1 |
| CombatLog | 5 | Низкая | 1 |
| NPC/Shop | 9 | Средняя | 1 |
| Интеграционные сценарии | 15 | Высокая | 1 |
| **Итого** | **~300** | | **30** |

---

## 7. Организация кода

```
src/test/
├── test_runner.h            # Фреймворк (существует)
├── test_all.h               # Include всех тестов (существует)
├── test_game_helpers.h      # НОВЫЙ: хелперы для game-тестов
├── test_aabb.h              # Существует
├── test_collision.h         # Существует
├── test_transform.h         # Существует
├── test_event_bus.h         # Существует
├── test_exception.h         # Существует
├── test_camera.h            # Существует
├── test_frustum.h           # Существует
├── test_delta_time.h        # Существует
├── test_game_state.h        # Существует
│
├── test_dice.h              # НОВЫЙ
├── test_character.h         # НОВЫЙ
├── test_combat_system.h     # НОВЫЙ
├── test_combat_handler.h    # НОВЫЙ
├── test_turn_queue.h        # НОВЫЙ
├── test_monster_manager.h   # НОВЫЙ
├── test_monster_factory.h   # НОВЫЙ
├── test_spell_system.h      # НОВЫЙ
├── test_status_effect.h     # НОВЫЙ
├── test_inventory.h         # НОВЫЙ
├── test_equipment.h         # НОВЫЙ
├── test_item_factory.h      # НОВЫЙ
├── test_dungeon.h           # НОВЫЙ
├── test_dungeon_gen.h       # НОВЫЙ
├── test_grid_position.h     # НОВЫЙ
├── test_turn_manager.h      # НОВЫЙ
├── test_encounter.h         # НОВЫЙ
├── test_experience.h        # НОВЫЙ
├── test_crafting.h          # НОВЫЙ
├── test_overworld.h         # НОВЫЙ
├── test_quest.h             # НОВЫЙ
├── test_save_load.h         # НОВЫЙ
├── test_action.h            # НОВЫЙ
├── test_data_manager.h      # НОВЫЙ
├── test_animation.h         # НОВЫЙ
├── test_combat_log.h        # НОВЫЙ
├── test_npc_shop.h          # НОВЫЙ
└── test_scenarios.h         # НОВЫЙ: интеграционные
```

`test_all.h` должен включать все новые файлы.

---

## 8. Как запускать

Текущий запуск (через `build.bat` с флагом `-DRUN_TESTS`):
```bash
build.bat test   # или build.bat с опцией tests
```

Для новых тестов потребуется:
1. Добавить `#include` новых файлов в `test_all.h`.
2. Убедиться, что линковщик не требует OpenGL/SDL (мокать заглушками при необходимости).
3. Либо вынести game-тесты в отдельную консольную утилиту без графики.

**Рекомендация**: сделать отдельную test target, которая компилирует только тесты + core/game без engine/graphics:

```bash
build.bat test_game   # Новая цель
```

---

## 9. Проверка результатов

- Все тесты должны проходить перед каждым коммитом.
- `test_results.log` — вердикт.
- При добавлении новой механики — **сначала тест, потом реализация** (TDD).
