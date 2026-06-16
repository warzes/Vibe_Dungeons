#pragma once

#include <vector>
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

	[[nodiscard]] const std::vector<Monster>& All() const noexcept
	{
		return m_monsters;
	}

	[[nodiscard]] Monster* FindInFront(GridPosition playerPos, Direction playerFacing);

	void UpdateAI(const Dungeon& dungeon);

private:
	std::vector<Monster> m_monsters;
};
