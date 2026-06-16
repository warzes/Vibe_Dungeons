#include "stdafx.h"
#include "game/data/skill_manager.h"
#include "core/json_data_manager.h"

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

Skill SkillManager::GetSkill(const std::string& abilityId)
{
	const json& j = JsonDataManager::Instance().GetAbilityData(abilityId);
	if (j.is_null() || !j.is_object())
	{
		return Skill{};
	}
	return Skill::FromJson(j);
}

std::vector<Skill> SkillManager::GetSkillsForClass(const std::string& classId)
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

std::vector<Skill> SkillManager::GetSkillsForClassAtLevel(
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

std::vector<Skill> SkillManager::GetPassiveSkillsForClassAtLevel(
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

std::vector<std::string> SkillManager::GetSkillIdsForClassAtLevel(
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

std::vector<std::string> SkillManager::GetNewSkillIdsForLevel(
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
