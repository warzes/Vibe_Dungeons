#pragma once

#include "game/combat/monster.h"
#include "game/dungeon/dungeon.h"

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

	void UpdateAI(const Dungeon& dungeon);

private:
	uint32_t m_nextId = 1;
	std::unordered_map<GridPosition, Monster> m_monsters;
};
