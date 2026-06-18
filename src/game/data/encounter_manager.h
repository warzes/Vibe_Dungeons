#pragma once

#include <string>
#include <vector>
#include <random>
#include "game/combat/monster_group.h"

class Dungeon;
class MonsterManager;

class EncounterManager final
{
public:
	void LoadEncounters();

	// Get random encounter for given floor
	[[nodiscard]] const EncounterTemplate* GetRandomEncounterForFloor(int32_t floor) const;

	// Spawn an encounter group in the dungeon
	void SpawnEncounter(
		const EncounterTemplate& encounter,
		GridPosition center,
		MonsterManager& monsterManager,
		const Dungeon& dungeon,
		int32_t floor
	);

	// Check if a group exists at a position
	[[nodiscard]] uint32_t GetGroupAt(GridPosition pos) const;

	// Get all monsters in a group
	[[nodiscard]] std::vector<uint32_t> GetGroupMonsters(uint32_t groupId) const;

	// Trigger group aggro
	void TriggerGroupAggro(uint32_t groupId);

	// Group initiative: all monsters in the same group act together
	[[nodiscard]] bool IsGroupActive(uint32_t groupId) const;

	void Clear();

private:
	std::vector<EncounterTemplate> m_encounters;
	std::vector<MonsterGroup> m_activeGroups;
	uint32_t m_nextGroupId = 1;
	std::mt19937 m_rng{ std::random_device{}() };

	[[nodiscard]] const MonsterGroup* findGroup(uint32_t groupId) const;
	[[nodiscard]] MonsterGroup* findGroup(uint32_t groupId);
	[[nodiscard]] MonsterGroup* findGroupByMonster(uint32_t monsterId);
};
