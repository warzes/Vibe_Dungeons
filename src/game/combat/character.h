#pragma once

#include "game/grid_position.h"
#include "game/direction.h"
#include "game/combat/inventory.h"
#include "game/data/skill_manager.h"

struct Character final
{
	static constexpr int32_t NUM_ACTION_SLOTS = 9;

	std::string name = "Hero";
	std::string charClass = "barbarian";
	int32_t level = 1;
	int32_t hp = 24;
	int32_t maxHp = 24;
	int32_t mp = 0;
	int32_t maxMp = 0;
	int32_t ac = 12;
	int32_t str = 16;
	int32_t dex = 10;
	int32_t con = 14;
	int32_t intel = 8;
	int32_t atkBonus = 3;
	int32_t damageBonus = 2;
	int32_t damageMin = 1;
	int32_t damageMax = 6;
	int32_t xp = 0;
	int32_t xpForNext = 100;
	GridPosition position;
	Direction facing = Direction::North;
	Inventory inventory;

	std::vector<std::string> unlockedSkills;
	std::array<ActionSlot, NUM_ACTION_SLOTS> actionSlots{};
	std::vector<std::string> learnedSpells;
};
