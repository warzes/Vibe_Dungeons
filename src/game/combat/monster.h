#pragma once

#include "game/grid_position.h"
#include "game/direction.h"
#include "game/combat/status_effect.h"

#include <string>
#include <vector>

enum class MonsterAI : uint8_t
{
	Stationary,
	Patrol,
	Aggressive
};

enum class MonsterRole : uint8_t
{
	Normal,
	Tank,
	Dps,
	Healer,
	Boss
};

struct Monster final
{
	uint32_t id = 0;
	std::string name = "Rat";
	std::string typeId = "slime";
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
	int32_t xpReward = 10;

	// ---- Ranged attack support (step 137) ----
	int32_t range = 1;                      // attack range in cells
	bool hasRangedAttack = false;
	int32_t rangedDamageMin = 0;
	int32_t rangedDamageMax = 0;

	// ---- Monster groups (step 158) ----
	uint32_t groupId = 0;
	uint32_t leaderId = 0;                  // group leader (step 162)
	MonsterRole role = MonsterRole::Normal;
	bool isBoss = false;
	std::string bossAbility;                // special boss-only ability

	// ---- Status effects (step 149) ----
	std::vector<ActiveEffect> activeEffects;

	// ---- Resistances / immunities (step 154) ----
	std::vector<std::string> resistances;
	std::vector<std::string> immunities;
};
