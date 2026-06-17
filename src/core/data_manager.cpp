#include "stdafx.h"
#include "core/data_manager.h"
#include "core/json_data_manager.h"

//==============================================================================
//  Skill
//==============================================================================

Skill Skill::FromJson(const json& j)
{
	Skill s;
	s.id = j.value("id", std::string());
	s.name = j.value("name", std::string());
	s.type = j.value("type", std::string());
	s.element = j.value("element", std::string());
	s.passive = j.value("passive", false);
	s.mpCost = j.value("mpCost", 0);
	s.cooldown = j.value("cooldown", 0);
	s.damageMin = j.value("damageMin", 0);
	s.damageMax = j.value("damageMax", 0);
	s.damageMult = j.value("damageMult", 1.0);
	s.atkBonus = j.value("atkBonus", 0);
	s.healMin = j.value("healMin", 0);
	s.healMax = j.value("healMax", 0);
	s.hpBonus = j.value("hpBonus", 0);
	s.mpBonus = j.value("mpBonus", 0);
	s.acBonus = j.value("acBonus", 0);
	s.damageBonus = j.value("damageBonus", 0);
	s.range = j.value("range", 1);
	s.radius = j.value("radius", 0);
	s.levelReq = j.value("levelReq", 1);
	s.description = j.value("description", std::string());

	if (j.contains("classReq") && j["classReq"].is_array())
	{
		for (const auto& c : j["classReq"])
		{
			s.classReq.push_back(c.get<std::string>());
		}
	}

	return s;
}

//==============================================================================
//  Class
//==============================================================================

const json& DataManager::GetClass(const std::string& classId)
{
	return JsonDataManager::Instance().GetClassData(classId);
}

void DataManager::ApplyStartingStats(const std::string& classId, int32_t& hp, int32_t& mp,
	int32_t& maxHp, int32_t& maxMp, int32_t& ac, int32_t& atkBonus,
	int32_t& str, int32_t& dex, int32_t& con, int32_t& intel)
{
	const json& data = JsonDataManager::Instance().GetClassData(classId);
	if (data.is_null() || data.empty())
	{
		return;
	}

	maxHp = data.value("baseHp", 20);
	hp = maxHp;
	maxMp = data.value("baseMp", 0);
	mp = maxMp;
	ac = 10 + data.value("acBonus", 0);
	atkBonus = data.value("atkBonus", 1);
	str = data.value("baseStr", 10);
	dex = data.value("baseDex", 10);
	con = data.value("baseCon", 10);
	intel = data.value("baseInt", 10);
}

//==============================================================================
//  Skill / Ability
//==============================================================================

Skill DataManager::GetSkill(const std::string& abilityId)
{
	const json& j = JsonDataManager::Instance().GetAbilityData(abilityId);
	if (j.is_null() || !j.is_object())
	{
		return Skill{};
	}
	return Skill::FromJson(j);
}

std::vector<Skill> DataManager::GetSkillsForClass(const std::string& classId)
{
	std::vector<Skill> result;
	const json& abilities = JsonDataManager::Instance().AllAbilities();
	if (!abilities.is_array())
	{
		return result;
	}

	for (const auto& j : abilities)
	{
		Skill s = Skill::FromJson(j);
		bool matches = false;
		for (const auto& req : s.classReq)
		{
			if (req == classId)
			{
				matches = true;
				break;
			}
		}
		if (matches || s.classReq.empty())
		{
			result.push_back(std::move(s));
		}
	}

	return result;
}

std::vector<Skill> DataManager::GetSkillsForClassAtLevel(
	const std::string& classId, int32_t level)
{
	std::vector<Skill> all = GetSkillsForClass(classId);
	std::vector<Skill> result;
	for (const auto& s : all)
	{
		if (s.levelReq <= level)
		{
			result.push_back(s);
		}
	}
	return result;
}

std::vector<Skill> DataManager::GetPassiveSkillsForClassAtLevel(
	const std::string& classId, int32_t level)
{
	std::vector<Skill> all = GetSkillsForClassAtLevel(classId, level);
	std::vector<Skill> result;
	for (const auto& s : all)
	{
		if (s.passive)
		{
			result.push_back(s);
		}
	}
	return result;
}

std::vector<std::string> DataManager::GetSkillIdsForClassAtLevel(
	const std::string& classId, int32_t level)
{
	std::vector<Skill> skills = GetSkillsForClassAtLevel(classId, level);
	std::vector<std::string> ids;
	for (const auto& s : skills)
	{
		ids.push_back(s.id);
	}
	return ids;
}

std::vector<std::string> DataManager::GetNewSkillIdsForLevel(
	const std::string& classId, int32_t newLevel,
	const std::vector<std::string>& alreadyUnlocked)
{
	std::vector<Skill> all = GetSkillsForClass(classId);
	std::vector<std::string> result;
	for (const auto& s : all)
	{
		if (s.levelReq == newLevel)
		{
			bool found = false;
			for (const auto& u : alreadyUnlocked)
			{
				if (u == s.id)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				result.push_back(s.id);
			}
		}
	}
	return result;
}

//==============================================================================
//  Ability (raw JSON)
//==============================================================================

const json& DataManager::GetAbility(const std::string& abilityId)
{
	return JsonDataManager::Instance().GetAbilityData(abilityId);
}

std::vector<std::string> DataManager::GetAbilitiesByClass(const std::string& classId)
{
	std::vector<std::string> result;
	const json& abilities = JsonDataManager::Instance().AllAbilities();
	for (const auto& ab : abilities)
	{
		if (ab.contains("classReq"))
		{
			for (const auto& req : ab["classReq"])
			{
				if (req.get<std::string>() == classId)
				{
					result.push_back(ab["id"].get<std::string>());
					break;
				}
			}
		}
	}
	return result;
}

//==============================================================================
//  Spell
//==============================================================================

const json& DataManager::GetSpell(const std::string& spellId)
{
	return JsonDataManager::Instance().GetSpellData(spellId);
}

std::vector<std::string> DataManager::GetSpellsByClass(const std::string& classId)
{
	std::vector<std::string> result;
	const json& spells = JsonDataManager::Instance().AllSpells();
	for (const auto& spell : spells)
	{
		if (spell.contains("classReq"))
		{
			for (const auto& req : spell["classReq"])
			{
				if (req.get<std::string>() == classId)
				{
					result.push_back(spell["id"].get<std::string>());
					break;
				}
			}
		}
	}
	return result;
}

//==============================================================================
//  Item stats
//==============================================================================

ItemStats DataManager::CalculateItemStats(
	const json& baseItem,
	const json& material,
	const json& prefix,
	const json& postfix)
{
	ItemStats stats;

	// Base values
	stats.damageMin = baseItem.value("damageMin", 0);
	stats.damageMax = baseItem.value("damageMax", 0);
	stats.ac = baseItem.value("ac", 0);

	// Material modifiers
	if (!material.is_null() && !material.empty())
	{
		float dmgMult = material.value("damageMult", 1.0f);
		stats.damageMin = static_cast<int32_t>(static_cast<float>(stats.damageMin) * dmgMult);
		stats.damageMax = static_cast<int32_t>(static_cast<float>(stats.damageMax) * dmgMult);
		stats.ac += material.value("acBonus", 0);
	}

	// Prefix modifiers
	if (!prefix.is_null() && !prefix.empty())
	{
		stats.damageMin += prefix.value("damageBonus", 0);
		stats.damageMax += prefix.value("damageBonus", 0);
		stats.atkBonus += prefix.value("atkBonus", 0);
		stats.ac += prefix.value("acBonus", 0);
	}

	// Postfix modifiers
	if (!postfix.is_null() && !postfix.empty())
	{
		stats.damageMin += postfix.value("damageBonus", 0);
		stats.damageMax += postfix.value("damageBonus", 0);
		stats.atkBonus += postfix.value("atkBonus", 0);
		stats.ac += postfix.value("acBonus", 0);
		stats.strBonus += postfix.value("strBonus", 0);
		stats.dexBonus += postfix.value("dexBonus", 0);
		stats.conBonus += postfix.value("conBonus", 0);
		stats.hpBonus += postfix.value("hpBonus", 0);
	}

	// Ensure minimums
	if (stats.damageMin < 1) { stats.damageMin = 1; }
	if (stats.damageMax < stats.damageMin) { stats.damageMax = stats.damageMin; }

	return stats;
}
