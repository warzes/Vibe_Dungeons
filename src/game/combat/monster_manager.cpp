#include "stdafx.h"
#include "game/combat/monster_manager.h"
#include "game/combat/status_effect.h"
#include "game/combat/character.h"
#include "game/ui/combat_log.h"
#include "game/data/skill_manager.h"

Monster* MonsterManager::FindById(uint32_t id)
{
	for (auto& pair : m_monsters)
	{
		if (pair.second.id == id)
		{
			return &pair.second;
		}
	}
	return nullptr;
}

const Monster* MonsterManager::FindById(uint32_t id) const
{
	for (const auto& pair : m_monsters)
	{
		if (pair.second.id == id)
		{
			return &pair.second;
		}
	}
	return nullptr;
}

void MonsterManager::Spawn(Monster monster)
{
	monster.id = m_nextId++;
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

std::vector<Monster*> MonsterManager::FindInLine(GridPosition from, Direction dir, int32_t range) const
{
	std::vector<Monster*> result;
	glm::ivec2 delta = DirectionToVec(dir);

	for (int32_t step = 1; step <= range; ++step)
	{
		GridPosition check = {
			from.row + delta.x * step,
			from.col + delta.y * step,
			from.floor
		};

		auto it = m_monsters.find(check);
		if (it != m_monsters.end() && it->second.alive)
		{
			result.push_back(const_cast<Monster*>(&it->second));
		}
	}

	return result;
}

std::vector<Monster*> MonsterManager::FindInRadius(GridPosition center, int32_t radius) const
{
	std::vector<Monster*> result;

	for (int32_t dr = -radius; dr <= radius; ++dr)
	{
		for (int32_t dc = -radius; dc <= radius; ++dc)
		{
			GridPosition check = {center.row + dr, center.col + dc, center.floor};
			auto it = m_monsters.find(check);
			if (it != m_monsters.end() && it->second.alive)
			{
				result.push_back(const_cast<Monster*>(&it->second));
			}
		}
	}

	return result;
}

std::vector<Monster*> MonsterManager::FindGroup(uint32_t groupId)
{
	std::vector<Monster*> result;
	for (auto& pair : m_monsters)
	{
		if (pair.second.groupId == groupId && pair.second.alive)
		{
			result.push_back(&pair.second);
		}
	}
	return result;
}

void MonsterManager::TriggerGroupAggro(uint32_t groupId, uint32_t attackedMonsterId)
{
	(void)attackedMonsterId;
	// Set all monsters in the group to Aggressive AI
	for (auto& pair : m_monsters)
	{
		if (pair.second.groupId == groupId && pair.second.alive)
		{
			pair.second.ai = MonsterAI::Aggressive;
		}
	}
}

bool MonsterManager::IsGroupAggroed(uint32_t groupId) const
{
	for (const auto& pair : m_monsters)
	{
		if (pair.second.groupId == groupId && pair.second.alive)
		{
			if (pair.second.ai == MonsterAI::Aggressive)
			{
				return true;
			}
		}
	}
	return false;
}

void MonsterManager::ProcessMonsterEffects(
	StatusSystem& statusSystem,
	CombatLog& combatLog)
{
	for (auto& pair : m_monsters)
	{
		Monster& mon = pair.second;
		if (!mon.alive) continue;
		if (mon.activeEffects.empty()) continue;

		// Tick damage over time effects
		std::vector<std::string> expired;
		statusSystem.ProcessTick(mon.activeEffects, expired,
			[&](int32_t damage, const std::string& effectName)
			{
				mon.hp -= damage;
				if (damage > 0)
				{
					combatLog.Add(mon.name + " takes " + std::to_string(damage) + " " + effectName + " damage!",
						glm::vec3(1.0f, 0.4f, 0.0f));
				}
				else
				{
					combatLog.Add(mon.name + " regenerates " + std::to_string(-damage) + " HP.",
						glm::vec3(0.2f, 1.0f, 0.2f));
				}

				if (mon.hp <= 0)
				{
					mon.hp = 0;
					mon.alive = false;
					combatLog.Add(mon.name + " dies from " + effectName + "!",
						glm::vec3(0.2f, 1.0f, 0.2f));
				}
			});

		// Advance effect timers
		statusSystem.AdvanceTurns(mon.activeEffects);
	}
}

void MonsterManager::UpdateAI(const Dungeon& dungeon, const Character& character)
{
	for (auto& pair : m_monsters)
	{
		Monster& mon = pair.second;
		if (!mon.alive)
		{
			continue;
		}

		// Step 162: Group leader-following logic
		// If this monster belongs to a group with a leader and is NOT the leader,
		// it should follow the leader (patrol formation)
		if (mon.groupId > 0 && mon.leaderId > 0 && mon.id != mon.leaderId)
		{
			// Find the leader
			Monster* leader = FindById(mon.leaderId);

			// If leader is dead, promote this monster to leader
			if (!leader || !leader->alive)
			{
				mon.leaderId = 0;
			}
			else
			{
				// Step 162: Follow the leader
				// If leader is aggressive, this monster also becomes aggressive
				if (leader->ai == MonsterAI::Aggressive && mon.ai != MonsterAI::Aggressive)
				{
					mon.ai = MonsterAI::Aggressive;
				}

				if (leader->ai == MonsterAI::Aggressive)
				{
					// When aggressive, follower helps attack the player
					GridPosition playerPos = character.GetPosition();
					int32_t dist = std::abs(mon.position.row - playerPos.row)
						+ std::abs(mon.position.col - playerPos.col);

					if (dist <= 1)
					{
						// Adjacent to player — stay for melee
						mon.facing = DirectionToTarget(mon.position, playerPos);
						continue;
					}

					if (dist > 1 && dist <= mon.range && mon.hasRangedAttack)
					{
						mon.facing = DirectionToTarget(mon.position, playerPos);
						continue;
					}

					// Move toward player
					glm::ivec2 dir = {
						(playerPos.row > mon.position.row) ? 1 : (playerPos.row < mon.position.row) ? -1 : 0,
						(playerPos.col > mon.position.col) ? 1 : (playerPos.col < mon.position.col) ? -1 : 0
					};

					GridPosition nextRow = {mon.position.row + dir.x, mon.position.col, mon.position.floor};
					if (dir.x != 0 && dungeon.IsWalkable(nextRow) && !At(nextRow))
					{
						m_monsters.erase(mon.position);
						mon.position = nextRow;
						mon.facing = DirectionToTarget(mon.position, playerPos);
						m_monsters[mon.position] = std::move(mon);
						continue;
					}

					GridPosition nextCol = {mon.position.row, mon.position.col + dir.y, mon.position.floor};
					if (dir.y != 0 && dungeon.IsWalkable(nextCol) && !At(nextCol))
					{
						m_monsters.erase(mon.position);
						mon.position = nextCol;
						mon.facing = DirectionToTarget(mon.position, playerPos);
						m_monsters[mon.position] = std::move(mon);
						continue;
					}
				}
				else
				{
					// Patrol mode: follow the leader
					int32_t distToLeader = std::abs(mon.position.row - leader->position.row)
						+ std::abs(mon.position.col - leader->position.col);

					// Stay within 2 cells of the leader
					if (distToLeader > 2)
					{
						// Move toward leader
						glm::ivec2 dir = {
							(leader->position.row > mon.position.row) ? 1 : (leader->position.row < mon.position.row) ? -1 : 0,
							(leader->position.col > mon.position.col) ? 1 : (leader->position.col < mon.position.col) ? -1 : 0
						};

						GridPosition nextRow = {mon.position.row + dir.x, mon.position.col, mon.position.floor};
						if (dir.x != 0 && dungeon.IsWalkable(nextRow) && !At(nextRow))
						{
							m_monsters.erase(mon.position);
							mon.position = nextRow;
							mon.facing = DirectionToTarget(mon.position, leader->position);
							m_monsters[mon.position] = std::move(mon);
							continue;
						}

						GridPosition nextCol = {mon.position.row, mon.position.col + dir.y, mon.position.floor};
						if (dir.y != 0 && dungeon.IsWalkable(nextCol) && !At(nextCol))
						{
							m_monsters.erase(mon.position);
							mon.position = nextCol;
							mon.facing = DirectionToTarget(mon.position, leader->position);
							m_monsters[mon.position] = std::move(mon);
							continue;
						}
					}
					else
					{
						// Near leader — face the same direction as leader
						mon.facing = leader->facing;
					}
				}
				continue;
			}
		}

		switch (mon.ai)
		{
			case MonsterAI::Stationary:
				// Does nothing
				break;

			case MonsterAI::Patrol:
			{
				// Simple patrol: periodically move toward player if visible
				GridPosition playerPos = character.GetPosition();
				int32_t dist = std::abs(mon.position.row - playerPos.row)
					+ std::abs(mon.position.col - playerPos.col);

				if (dist <= 5 && dungeon.HasLineOfSight(mon.position, playerPos))
				{
					// Chase player slowly
					glm::ivec2 dir = {
						(playerPos.row > mon.position.row) ? 1 : (playerPos.row < mon.position.row) ? -1 : 0,
						(playerPos.col > mon.position.col) ? 1 : (playerPos.col < mon.position.col) ? -1 : 0
					};

					GridPosition next = {mon.position.row + dir.x, mon.position.col + dir.y, mon.position.floor};
					if (dungeon.IsWalkable(next) && !At(next))
					{
						m_monsters.erase(mon.position);
						mon.position = next;
						m_monsters[mon.position] = std::move(mon);
						break;
					}
				}
				break;
			}

			case MonsterAI::Aggressive:
			{
				// Move toward player, melee if adjacent, ranged if has bow
				GridPosition playerPos = character.GetPosition();
				int32_t dist = std::abs(mon.position.row - playerPos.row)
					+ std::abs(mon.position.col - playerPos.col);

				if (dist <= 1)
				{
					// Already adjacent - don't move
					break;
				}

				if (dist > 1 && dist <= mon.range && mon.hasRangedAttack)
				{
					// In ranged attack range - don't move
					// Facing toward player
					mon.facing = DirectionToTarget(mon.position, playerPos);
					break;
				}

				// Move toward player
				glm::ivec2 dir = {
					(playerPos.row > mon.position.row) ? 1 : (playerPos.row < mon.position.row) ? -1 : 0,
					(playerPos.col > mon.position.col) ? 1 : (playerPos.col < mon.position.col) ? -1 : 0
				};

				// Try to move in one axis first (pathfinding simplification)
				GridPosition nextRow = {mon.position.row + dir.x, mon.position.col, mon.position.floor};
				if (dir.x != 0 && dungeon.IsWalkable(nextRow) && !At(nextRow))
				{
					m_monsters.erase(mon.position);
					mon.position = nextRow;
					mon.facing = DirectionToTarget(mon.position, playerPos);
					m_monsters[mon.position] = std::move(mon);
					break;
				}

				GridPosition nextCol = {mon.position.row, mon.position.col + dir.y, mon.position.floor};
				if (dir.y != 0 && dungeon.IsWalkable(nextCol) && !At(nextCol))
				{
					m_monsters.erase(mon.position);
					mon.position = nextCol;
					mon.facing = DirectionToTarget(mon.position, playerPos);
					m_monsters[mon.position] = std::move(mon);
					break;
				}
				break;
			}
		}
	}
}
