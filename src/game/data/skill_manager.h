#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct Skill final
{
	std::string id;
	std::string name;
	std::string type;
	std::string element;
	bool passive = false;

	int32_t mpCost = 0;
	int32_t cooldown = 0;
	int32_t cooldownRemaining = 0;

	int32_t damageMin = 0;
	int32_t damageMax = 0;
	double damageMult = 1.0;
	int32_t atkBonus = 0;

	int32_t healMin = 0;
	int32_t healMax = 0;

	int32_t hpBonus = 0;
	int32_t mpBonus = 0;
	int32_t acBonus = 0;
	int32_t damageBonus = 0;

	int32_t range = 1;
	int32_t levelReq = 1;
	std::vector<std::string> classReq;

	std::string description;

	static Skill FromJson(const json& j);
};

struct ActionSlot final
{
	std::string type;  // "ability", "spell", "item"
	std::string id;
	int32_t cooldownRemaining = 0;
};

class SkillManager final
{
public:
	[[nodiscard]] static Skill GetSkill(const std::string& abilityId);

	[[nodiscard]] static std::vector<Skill> GetSkillsForClass(const std::string& classId);

	[[nodiscard]] static std::vector<Skill> GetSkillsForClassAtLevel(
		const std::string& classId, int32_t level);

	[[nodiscard]] static std::vector<Skill> GetPassiveSkillsForClassAtLevel(
		const std::string& classId, int32_t level);

	[[nodiscard]] static std::vector<std::string> GetSkillIdsForClassAtLevel(
		const std::string& classId, int32_t level);

	[[nodiscard]] static std::vector<std::string> GetNewSkillIdsForLevel(
		const std::string& classId, int32_t newLevel,
		const std::vector<std::string>& alreadyUnlocked);
};
