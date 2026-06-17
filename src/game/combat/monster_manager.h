#pragma once

#include "game/combat/monster.h"
#include "game/dungeon/dungeon.h"

class MonsterManager final
{
public:
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
	std::unordered_map<GridPosition, Monster> m_monsters;
};
