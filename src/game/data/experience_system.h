#pragma once

#include "game/combat/character.h"
#include "game/combat/monster.h"

struct ExperienceSystem final
{
	static void AwardKill(Character& character, const Monster& monster);
	static bool CheckLevelUp(const Character& character);
	static int32_t XpForLevel(int32_t level);
	static int32_t HpGainForLevel(const std::string& charClass);

	static std::vector<std::string> GrantSkillsForLevel(
		Character& character, int32_t newLevel);

	static void ApplyPassiveSkills(Character& character);
};
