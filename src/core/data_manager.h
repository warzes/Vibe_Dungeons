#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "core/json_alias.h"

// ---- Skill ----
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

// ---- ActionSlot ----
struct ActionSlot final
{
	std::string type;  // "ability", "spell", "item"
	std::string id;
	int32_t cooldownRemaining = 0;
};

// ---- ItemStats ----
struct ItemStats final
{
	int32_t damageMin = 0;
	int32_t damageMax = 0;
	int32_t ac = 0;
	int32_t atkBonus = 0;
	int32_t strBonus = 0;
	int32_t dexBonus = 0;
	int32_t conBonus = 0;
	int32_t hpBonus = 0;
	int32_t mpBonus = 0;
};

// ---- Unified DataManager ----
struct DataManager final
{
	// Class
	[[nodiscard]] static const json& GetClass(const std::string& classId);
	static void ApplyStartingStats(const std::string& classId, int32_t& hp, int32_t& mp,
		int32_t& maxHp, int32_t& maxMp, int32_t& ac, int32_t& atkBonus,
		int32_t& str, int32_t& dex, int32_t& con, int32_t& intel);

	// Skill / Ability
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

	// Ability (raw JSON)
	[[nodiscard]] static const json& GetAbility(const std::string& abilityId);
	[[nodiscard]] static std::vector<std::string> GetAbilitiesByClass(const std::string& classId);

	// Spell
	[[nodiscard]] static const json& GetSpell(const std::string& spellId);
	[[nodiscard]] static std::vector<std::string> GetSpellsByClass(const std::string& classId);

	// Item stats
	[[nodiscard]] static ItemStats CalculateItemStats(
		const json& baseItem,
		const json& material,
		const json& prefix,
		const json& postfix);
};
