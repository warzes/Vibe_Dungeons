#include "stdafx.h"
#include "game/combat/monster_manager.h"

void MonsterManager::Spawn(Monster monster)
{
	m_monsters.push_back(std::move(monster));
}

void MonsterManager::Despawn(GridPosition pos)
{
	std::erase_if(m_monsters, [&](const Monster& m)
	{
		return m.alive
			&& m.position.row == pos.row
			&& m.position.col == pos.col
			&& m.position.floor == pos.floor;
	});
}

void MonsterManager::RemoveDead()
{
	std::erase_if(m_monsters, [](const Monster& m) { return !m.alive; });
}

void MonsterManager::Clear()
{
	m_monsters.clear();
}

Monster* MonsterManager::At(GridPosition pos)
{
	for (auto& m : m_monsters)
	{
		if (m.alive
			&& m.position.row == pos.row
			&& m.position.col == pos.col
			&& m.position.floor == pos.floor)
		{
			return &m;
		}
	}
	return nullptr;
}

const Monster* MonsterManager::At(GridPosition pos) const
{
	for (const auto& m : m_monsters)
	{
		if (m.alive
			&& m.position.row == pos.row
			&& m.position.col == pos.col
			&& m.position.floor == pos.floor)
		{
			return &m;
		}
	}
	return nullptr;
}

Monster* MonsterManager::FindInFront(GridPosition playerPos, Direction playerFacing)
{
	glm::ivec2 delta = DirectionToVec(playerFacing);
	GridPosition front = {
		playerPos.row + delta.x,
		playerPos.col + delta.y,
		playerPos.floor
	};
	return At(front);
}

void MonsterManager::UpdateAI(const Dungeon& dungeon)
{
	(void)dungeon;
}
