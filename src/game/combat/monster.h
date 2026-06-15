#pragma once

#include <cstdint>
#include <string>
#include "game/grid_position.h"
#include "game/direction.h"

enum class MonsterAI : uint8_t
{
	Stationary,
	Patrol,
	Aggressive
};

enum class MonsterType : uint8_t
{
	Skeleton,
	Slime
};

struct Monster final
{
	std::string name = "Rat";
	MonsterType type = MonsterType::Skeleton;
	int32_t level = 1;
	int32_t hp = 5;
	int32_t maxHp = 5;
	int32_t ac = 10;
	int32_t atkBonus = 0;
	int32_t damageMin = 1;
	int32_t damageMax = 3;
	GridPosition position;
	Direction facing = Direction::North;
	MonsterAI ai = MonsterAI::Stationary;
	bool alive = true;
};
