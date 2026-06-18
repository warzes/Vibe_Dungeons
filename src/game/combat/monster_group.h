#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "game/grid_position.h"
#include "game/combat/monster.h"

enum class TargetingMode : uint8_t
{
	Single,
	Beam,
	AoE_3x3,
	AoE_5x5,
	Cross,
	Line
};

struct MonsterGroup final
{
	uint32_t groupId = 0;
	std::vector<GridPosition> positions;
	std::vector<uint32_t> monsterIds;   // monster instance IDs
	bool aggroed = false;               // group aggro state
	uint32_t leaderId = 0;              // leader monster instance ID (follows leader)

	// Encounter template (for JSON loading)
	std::string encounterId;             // references encounter_groups.json
};

struct AoeTarget final
{
	GridPosition center;
	int32_t radius = 0;
	TargetingMode mode = TargetingMode::Single;
	std::vector<GridPosition> affectedCells;
	std::vector<uint32_t> affectedMonsterIds;
	bool friendlyFire = false;     // true if player is in AoE
	bool hitWall = false;
};

struct EncounterEntry final
{
	std::string monsterTypeId;         // references monsters.json id
	int32_t count = 1;
	int32_t minLevel = 1;
	int32_t maxLevel = 1;
	MonsterRole role = MonsterRole::Normal;
	GridPosition spawnOffset;          // relative to encounter center
};

struct EncounterTemplate final
{
	std::string id;
	std::string name;                  // "Goblin Patrol", "Skeleton Squad"
	int32_t minFloor = 1;
	int32_t maxFloor = 1;
	std::vector<EncounterEntry> entries;
	int32_t minGroupSize = 1;
	int32_t maxGroupSize = 6;
	int32_t weight = 10;               // spawn weight (higher = more common)
};
