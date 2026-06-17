#include "stdafx.h"
#include "game/data/experience_system.h"
#include "core/json_data_manager.h"
#include "game/combat/dice.h"
#include "game/data/skill_manager.h"
#include "game/combat/monster.h"

void ExperienceSystem::AwardKill(Character& character, const Monster& monster)
{
	character.AddXp(monster.xpReward);

	int32_t needed = XpForLevel(character.GetLevel() + 1);
	if (needed > 0)
	{
		character.SetXpForNext(needed);
	}
}

bool ExperienceSystem::CheckLevelUp(const Character& character)
{
	int32_t needed = XpForLevel(character.GetLevel() + 1);
	if (needed <= 0)
	{
		return false;
	}
	return character.GetXp() >= needed;
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

std::vector<std::string> ExperienceSystem::GrantSkillsForLevel(
	Character& character, int32_t newLevel)
{
	std::vector<std::string> newSkills = SkillManager::GetNewSkillIdsForLevel(
		character.GetClass(), newLevel, character.GetUnlockedSkills());

	for (const auto& skillId : newSkills)
	{
		character.GetUnlockedSkills().push_back(skillId);
	}

	// Auto-assign new non-passive active skills to empty action slots
	int32_t slotIdx = 0;
	for (const auto& skillId : newSkills)
	{
		Skill s = SkillManager::GetSkill(skillId);
		if (!s.passive && slotIdx < Character::NUM_ACTION_SLOTS)
		{
			// Find first empty slot
			while (slotIdx < Character::NUM_ACTION_SLOTS &&
				!character.GetActionSlots()[slotIdx].id.empty())
			{
				++slotIdx;
			}
			if (slotIdx < Character::NUM_ACTION_SLOTS)
			{
				character.GetActionSlots()[slotIdx].type = "ability";
				character.GetActionSlots()[slotIdx].id = skillId;
				character.GetActionSlots()[slotIdx].cooldownRemaining = 0;
				++slotIdx;
			}
		}
	}

	return newSkills;
}

void ExperienceSystem::ApplyPassiveSkills(Character& character)
{
	std::vector<Skill> passives = SkillManager::GetPassiveSkillsForClassAtLevel(
		character.GetClass(), character.GetLevel());

	// Reset bonuses from passives (reapply all)
	// First, clear passive-applied bonuses by rebuilding from base class data
	const json& classData = JsonDataManager::Instance().GetClassData(character.GetClass());
	int32_t baseHp = classData.value("baseHp", 20);
	int32_t baseMp = classData.value("baseMp", 0);

	// Reset to base + level gains
	character.SetMaxHp(baseHp + (character.GetLevel() - 1) * classData.value("hpPerLevel", 6));
	character.SetMaxMp(baseMp + (character.GetLevel() - 1) * classData.value("mpPerLevel", 0));
	character.SetAtkBonus(classData.value("atkBonus", 0));
	character.SetAc(10 + classData.value("acBonus", 0));
	character.SetDamageBonus(classData.value("damageBonus", 0));

	for (const auto& s : passives)
	{
		character.SetMaxHp(character.GetMaxHp() + s.hpBonus);
		character.SetMaxMp(character.GetMaxMp() + s.mpBonus);
		character.SetAtkBonus(character.GetAtkBonus() + s.atkBonus);
		character.SetAc(character.GetAc() + s.acBonus);
		character.SetDamageBonus(character.GetDamageBonus() + s.damageBonus);
	}

	// Clamp
	if (character.GetHp() > character.GetMaxHp())
	{
		character.SetHp(character.GetMaxHp());
	}
	if (character.GetMp() > character.GetMaxMp())
	{
		character.SetMp(character.GetMaxMp());
	}
}
