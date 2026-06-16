#include "stdafx.h"
#include "game/data/experience_system.h"
#include "game/combat/character.h"
#include "game/combat/monster.h"
#include "core/json_data_manager.h"
#include "game/combat/dice.h"

void ExperienceSystem::AwardKill(Character& character, const Monster& monster)
{
	character.xp += monster.xpReward;

	int32_t needed = XpForLevel(character.level + 1);
	if (needed > 0)
	{
		character.xpForNext = needed;
	}
}

bool ExperienceSystem::CheckLevelUp(const Character& character)
{
	int32_t needed = XpForLevel(character.level + 1);
	if (needed <= 0)
	{
		return false;
	}
	return character.xp >= needed;
}

int32_t ExperienceSystem::XpForLevel(int32_t level)
{
	const json& table = JsonDataManager::Instance().GetLevelTable();
	if (!table.is_array())
	{
		return level * 100;
	}

	for (const auto& entry : table)
	{
		if (entry.value("level", 0) == level)
		{
			return entry.value("xpNeeded", level * 100);
		}
	}

	return level * 100;
}

int32_t ExperienceSystem::HpGainForLevel(const std::string& charClass)
{
	const json& classData = JsonDataManager::Instance().GetClassData(charClass);
	int32_t hitDice = classData.value("hitDice", 8);
	return Dice::Roll(1, hitDice);
}
