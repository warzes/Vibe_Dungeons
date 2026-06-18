#pragma once

#include "game/combat/monster.h"
#include "game/dungeon/dungeon.h"
#include "game/combat/monster_group.h"

class MonsterManager final
{
public:
	[[nodiscard]] Monster* FindById(uint32_t id);
	[[nodiscard]] const Monster* FindById(uint32_t id) const;

	void Spawn(Monster monster);

	void Despawn(GridPosition pos);

	void RemoveDead();

	void Clear();

	[[nodiscard]] Monster* At(GridPosition pos);
	[[nodiscard]] const Monster* At(GridPosition pos) const;

	[[nodiscard]] std::vector<Monster> All() const;

	[[nodiscard]] Monster* FindInFront(GridPosition playerPos, Direction playerFacing);

	// Find all monsters in a line (for beam/line spells)
	[[nodiscard]] std::vector<Monster*> FindInLine(GridPosition from, Direction dir, int32_t range) const;

	// Find all monsters in radius around a point
	[[nodiscard]] std::vector<Monster*> FindInRadius(GridPosition center, int32_t radius) const;

	// Group support (step 161)
	[[nodiscard]] std::vector<Monster*> FindGroup(uint32_t groupId);
	void TriggerGroupAggro(uint32_t groupId, uint32_t attackedMonsterId);
	[[nodiscard]] bool IsGroupAggroed(uint32_t groupId) const;

	// Status effect processing
	void ProcessMonsterEffects(
		StatusSystem& statusSystem,
		class CombatLog& combatLog
	);

	void UpdateAI(const Dungeon& dungeon, const class Character& character);

private:
	uint32_t m_nextId = 1;
	std::unordered_map<GridPosition, Monster> m_monsters;
};
