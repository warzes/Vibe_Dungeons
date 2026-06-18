#include "stdafx.h"
#include "game/data/encounter_manager.h"
#include "core/json_data_manager.h"
#include "game/dungeon/dungeon.h"
#include "game/combat/monster_manager.h"
#include "game/data/monster_factory.h"

void EncounterManager::LoadEncounters()
{
	m_encounters.clear();

	// Built-in encounters
	auto addEncounter = [&](std::string id, std::string name, int32_t minF, int32_t maxF,
	                         std::vector<EncounterEntry> entries, int32_t weight)
	{
		EncounterTemplate et;
		et.id = std::move(id);
		et.name = std::move(name);
		et.minFloor = minF;
		et.maxFloor = maxF;
		et.entries = std::move(entries);
		et.weight = weight;
		et.minGroupSize = 1;
		et.maxGroupSize = 6;
		m_encounters.push_back(std::move(et));
	};

	// Floor 1 encounters
	addEncounter("rat_pack", "Rat Pack", 1, 1, {
		{"giant_rat", 2, 1, 1, MonsterRole::Normal, {0, 1, 0}},
		{"giant_rat", 1, 1, 1, MonsterRole::Normal, {1, 0, 0}}
	}, 20);

	addEncounter("slime_puddle", "Slime Puddle", 1, 1, {
		{"slime", 2, 1, 1, MonsterRole::Normal, {0, 1, 0}},
		{"slime", 1, 1, 1, MonsterRole::Normal, {1, 0, 0}}
	}, 20);

	addEncounter("goblin_patrol", "Goblin Patrol", 1, 2, {
		{"goblin", 2, 1, 1, MonsterRole::Dps, {0, 1, 0}},
		{"goblin", 1, 1, 1, MonsterRole::Normal, {-1, 0, 0}}
	}, 15);

	addEncounter("skeleton_watch", "Skeleton Watch", 1, 2, {
		{"skeleton", 2, 1, 1, MonsterRole::Normal, {0, 1, 0}},
		{"skeleton", 1, 1, 1, MonsterRole::Normal, {1, 0, 0}}
	}, 15);

	// Floor 2 encounters
	addEncounter("skeleton_squad", "Skeleton Squad", 2, 3, {
		{"skeleton_warrior", 1, 2, 2, MonsterRole::Tank, {0, 0, 0}},
		{"skeleton", 2, 2, 2, MonsterRole::Dps, {0, 1, 0}},
		{"skeleton_mage", 1, 2, 2, MonsterRole::Normal, {1, 0, 0}}
	}, 15);

	addEncounter("elemental_blaze", "Elemental Blaze", 2, 3, {
		{"fire_elemental", 1, 3, 3, MonsterRole::Normal, {0, 0, 0}},
		{"slime", 2, 2, 2, MonsterRole::Normal, {0, 1, 0}}
	}, 10);

	// Floor 3 encounters
	addEncounter("mage_cabal", "Mage Cabal", 3, 3, {
		{"skeleton_mage", 2, 3, 3, MonsterRole::Dps, {0, 1, 0}},
		{"skeleton_warrior", 1, 3, 3, MonsterRole::Tank, {0, 0, 0}},
		{"skeleton", 1, 3, 3, MonsterRole::Normal, {1, 0, 0}}
	}, 12);

	// Boss encounter
	addEncounter("skeleton_lord", "Skeleton Lord", 3, 3, {
		{"skeleton_warrior", 1, 3, 3, MonsterRole::Boss, {0, 0, 0}},
		{"skeleton_mage", 2, 3, 3, MonsterRole::Dps, {0, 1, 0}},
		{"skeleton_warrior", 2, 3, 3, MonsterRole::Tank, {1, 0, 0}},
		{"skeleton", 2, 3, 3, MonsterRole::Normal, {-1, 0, 0}}
	}, 5);

	// Load from JSON if available
	try
	{
		const json& encountersJson = JsonDataManager::Instance().AllEncounters();
		if (encountersJson.is_array())
		{
			for (const auto& j : encountersJson)
			{
				EncounterTemplate et;
				et.id = j.value("id", std::string());
				et.name = j.value("name", et.id);
				et.minFloor = j.value("minFloor", 1);
				et.maxFloor = j.value("maxFloor", 1);
				et.weight = j.value("weight", 10);

				if (j.contains("entries") && j["entries"].is_array())
				{
					for (const auto& ej : j["entries"])
					{
						EncounterEntry ee;
						ee.monsterTypeId = ej.value("monsterId", std::string());
						ee.count = ej.value("count", 1);
						ee.minLevel = ej.value("minLevel", 1);
						ee.maxLevel = ej.value("maxLevel", 1);

						std::string roleStr = ej.value("role", "Normal");
						if (roleStr == "Tank")       ee.role = MonsterRole::Tank;
						else if (roleStr == "Dps")   ee.role = MonsterRole::Dps;
						else if (roleStr == "Healer") ee.role = MonsterRole::Healer;
						else if (roleStr == "Boss")  ee.role = MonsterRole::Boss;
						else                         ee.role = MonsterRole::Normal;

						if (ej.contains("spawnOffset") && ej["spawnOffset"].is_array() && ej["spawnOffset"].size() >= 2)
						{
							ee.spawnOffset.row = ej["spawnOffset"][0].get<int32_t>();
							ee.spawnOffset.col = ej["spawnOffset"][1].get<int32_t>();
						}

						et.entries.push_back(std::move(ee));
					}
				}

				m_encounters.push_back(std::move(et));
			}
		}
	}
	catch (...)
	{
		// Use built-in encounters
	}
}

const EncounterTemplate* EncounterManager::GetRandomEncounterForFloor(int32_t floor) const
{
	// Collect all valid encounters for this floor
	std::vector<const EncounterTemplate*> valid;
	for (const auto& e : m_encounters)
	{
		if (floor >= e.minFloor && floor <= e.maxFloor)
		{
			valid.push_back(&e);
		}
	}

	if (valid.empty())
	{
		return nullptr;
	}

	// Weighted random selection
	int32_t totalWeight = 0;
	for (const auto* e : valid) { totalWeight += e->weight; }

	std::mt19937 rng{ std::random_device{}() };
	int32_t roll = std::uniform_int_distribution<int32_t>(0, totalWeight - 1)(rng);

	int32_t accum = 0;
	for (const auto* e : valid)
	{
		accum += e->weight;
		if (roll < accum)
		{
			return e;
		}
	}

	return valid.empty() ? nullptr : valid.back();
}

void EncounterManager::SpawnEncounter(
	const EncounterTemplate& encounter,
	GridPosition center,
	MonsterManager& monsterManager,
	const Dungeon& dungeon,
	int32_t /*floor*/)
{
	MonsterGroup group;
	group.groupId = m_nextGroupId++;
	group.encounterId = encounter.id;
	group.positions.push_back(center);

	int32_t monsterIndex = 0;
	for (const auto& entry : encounter.entries)
	{
		for (int32_t i = 0; i < entry.count; ++i)
		{
			GridPosition spawnPos = center;
			if (i > 0)
			{
				// Spread out from center
				spawnPos.row += entry.spawnOffset.row + i;
				spawnPos.col += entry.spawnOffset.col;
			}

			// Find a walkable position near the spawn point
			if (!dungeon.IsWalkable(spawnPos))
			{
				// Try neighbors
				const glm::ivec2 dirs[] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
				bool placed = false;
				for (const auto& d : dirs)
				{
					GridPosition neighbor = {spawnPos.row + d.x, spawnPos.col + d.y, spawnPos.floor};
					if (dungeon.IsWalkable(neighbor) && !monsterManager.At(neighbor))
					{
						spawnPos = neighbor;
						placed = true;
						break;
					}
				}
				if (!placed) continue;
			}

			// Don't spawn on top of another monster
			if (monsterManager.At(spawnPos))
			{
				continue;
			}

			int32_t level = entry.minLevel + (std::rand() % (entry.maxLevel - entry.minLevel + 1));
			Monster mon = MonsterFactory::Create(entry.monsterTypeId, level);
			mon.position = spawnPos;
			mon.role = entry.role;
			mon.groupId = group.groupId;

			monsterManager.Spawn(std::move(mon));

			Monster* spawned = monsterManager.At(spawnPos);
			if (spawned)
			{
				group.monsterIds.push_back(spawned->id);
				group.positions.push_back(spawnPos);

				if (monsterIndex == 0)
				{
					group.leaderId = spawned->id;
				}

				// Step 162: propagate leaderId to all group members
				spawned->leaderId = group.leaderId;
				monsterIndex++;
			}
		}
	}

	if (!group.monsterIds.empty())
	{
		m_activeGroups.push_back(std::move(group));
	}
}

uint32_t EncounterManager::GetGroupAt(GridPosition pos) const
{
	for (const auto& g : m_activeGroups)
	{
		for (const auto& gp : g.positions)
		{
			if (gp == pos) return g.groupId;
		}
	}
	return 0;
}

std::vector<uint32_t> EncounterManager::GetGroupMonsters(uint32_t groupId) const
{
	const MonsterGroup* g = findGroup(groupId);
	if (g) return g->monsterIds;
	return {};
}

void EncounterManager::TriggerGroupAggro(uint32_t groupId)
{
	MonsterGroup* g = findGroup(groupId);
	if (g) g->aggroed = true;
}

bool EncounterManager::IsGroupActive(uint32_t groupId) const
{
	const MonsterGroup* g = findGroup(groupId);
	return g != nullptr;
}

void EncounterManager::Clear()
{
	m_activeGroups.clear();
}

const MonsterGroup* EncounterManager::findGroup(uint32_t groupId) const
{
	for (const auto& g : m_activeGroups)
	{
		if (g.groupId == groupId) return &g;
	}
	return nullptr;
}

MonsterGroup* EncounterManager::findGroup(uint32_t groupId)
{
	for (auto& g : m_activeGroups)
	{
		if (g.groupId == groupId) return &g;
	}
	return nullptr;
}

MonsterGroup* EncounterManager::findGroupByMonster(uint32_t monsterId)
{
	for (auto& g : m_activeGroups)
	{
		for (auto id : g.monsterIds)
		{
			if (id == monsterId) return &g;
		}
	}
	return nullptr;
}
