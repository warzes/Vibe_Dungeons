#pragma once

#include <cstdint>
#include <string>

struct Character;
class Monster;

struct ExperienceSystem final
{
	static void AwardKill(Character& character, const Monster& monster);
	static bool CheckLevelUp(const Character& character);
	static int32_t XpForLevel(int32_t level);
	static int32_t HpGainForLevel(const std::string& charClass);
};
