#include "stdafx.h"
#include "game/combat/monster_manager.h"

void MonsterManager::Spawn(Monster monster)
{
	m_monsters.insert_or_assign(monster.position, std::move(monster));
}

void MonsterManager::Despawn(GridPosition pos)
{
	m_monsters.erase(pos);
}

void MonsterManager::RemoveDead()
{
	std::erase_if(m_monsters, [](const auto& pair) { return !pair.second.alive; });
}

void MonsterManager::Clear()
{
	m_monsters.clear();
}

Monster* MonsterManager::At(GridPosition pos)
{
	auto it = m_monsters.find(pos);
	return (it != m_monsters.end() && it->second.alive) ? &it->second : nullptr;
}

const Monster* MonsterManager::At(GridPosition pos) const
{
	auto it = m_monsters.find(pos);
	return (it != m_monsters.end() && it->second.alive) ? &it->second : nullptr;
}

std::vector<Monster> MonsterManager::All() const
{
	std::vector<Monster> result;
	result.reserve(m_monsters.size());
	for (const auto& pair : m_monsters)
	{
		result.push_back(pair.second);
	}
	return result;
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
