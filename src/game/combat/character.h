#pragma once

#include "game/grid_position.h"
#include "game/direction.h"
#include "game/combat/inventory.h"

struct Character final
{
	std::string name = "Hero";
	int32_t level = 1;
	int32_t hp = 20;
	int32_t maxHp = 20;
	int32_t ac = 12;
	int32_t str = 12;
	int32_t dex = 12;
	int32_t con = 12;
	int32_t atkBonus = 2;
	int32_t damageMin = 1;
	int32_t damageMax = 6;
	GridPosition position;
	Direction facing = Direction::North;
	Inventory inventory;
};
