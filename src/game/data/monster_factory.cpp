#include "stdafx.h"
#include "game/data/monster_factory.h"
#include "core/json_data_manager.h"

Monster MonsterFactory::Create(const std::string& typeId, int32_t level)
{
	const json& data = JsonDataManager::Instance().GetMonsterData(typeId);

	Monster m;
	m.typeId = typeId;
	m.name = data.value("name", typeId);
	m.level = level;
	m.hp = data.value("hp", 5) + level * 2;
	m.maxHp = m.hp;
	m.ac = data.value("ac", 10);
	m.atkBonus = data.value("atkBonus", 0) + level / 2;
	m.damageMin = data.value("damageMin", 1);
	m.damageMax = data.value("damageMax", 3);
	m.xpReward = data.value("xpReward", 10) + level * 5;

	// Ranged attack support (step 137)
	m.hasRangedAttack = data.value("hasRangedAttack", false);
	m.rangedDamageMin = data.value("rangedDamageMin", 0);
	m.rangedDamageMax = data.value("rangedDamageMax", 0);
	m.range = data.value("range", 1);

	// Boss flag (step 164)
	m.isBoss = data.value("isBoss", false);
	m.bossAbility = data.value("bossAbility", std::string());

	// Resistances and immunities (step 154)
	if (data.contains("resistances") && data["resistances"].is_array())
	{
		for (const auto& r : data["resistances"])
		{
			m.resistances.push_back(r.get<std::string>());
		}
	}
	if (data.contains("immunities") && data["immunities"].is_array())
	{
		for (const auto& r : data["immunities"])
		{
			m.immunities.push_back(r.get<std::string>());
		}
	}

	std::string aiStr = data.value("ai", "Stationary");
	if (aiStr == "Patrol")
	{
		m.ai = MonsterAI::Patrol;
	}
	else if (aiStr == "Aggressive")
	{
		m.ai = MonsterAI::Aggressive;
	}
	else
	{
		m.ai = MonsterAI::Stationary;
	}

	return m;
}
