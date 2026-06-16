#include "stdafx.h"
#include "game/data/experience_system.h"
#include "game/combat/character.h"
#include "game/combat/monster.h"
#include "core/json_data_manager.h"
#include "game/combat/dice.h"
#include "game/data/skill_manager.h"

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

std::vector<std::string> ExperienceSystem::GrantSkillsForLevel(
	Character& character, int32_t newLevel)
{
	std::vector<std::string> newSkills = SkillManager::GetNewSkillIdsForLevel(
		character.charClass, newLevel, character.unlockedSkills);

	for (const auto& skillId : newSkills)
	{
		character.unlockedSkills.push_back(skillId);
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
				!character.actionSlots[slotIdx].id.empty())
			{
				++slotIdx;
			}
			if (slotIdx < Character::NUM_ACTION_SLOTS)
			{
				character.actionSlots[slotIdx].type = "ability";
				character.actionSlots[slotIdx].id = skillId;
				character.actionSlots[slotIdx].cooldownRemaining = 0;
				++slotIdx;
			}
		}
	}

	return newSkills;
}

void ExperienceSystem::ApplyPassiveSkills(Character& character)
{
	std::vector<Skill> passives = SkillManager::GetPassiveSkillsForClassAtLevel(
		character.charClass, character.level);

	// Reset bonuses from passives (reapply all)
	// First, clear passive-applied bonuses by rebuilding from base class data
	const json& classData = JsonDataManager::Instance().GetClassData(character.charClass);
	int32_t baseHp = classData.value("baseHp", 20);
	int32_t baseMp = classData.value("baseMp", 0);

	// Reset to base + level gains
	character.maxHp = baseHp + (character.level - 1) * classData.value("hpPerLevel", 6);
	character.maxMp = baseMp + (character.level - 1) * classData.value("mpPerLevel", 0);
	character.atkBonus = classData.value("atkBonus", 0);
	character.ac = 10 + classData.value("acBonus", 0);
	character.damageBonus = classData.value("damageBonus", 0);

	for (const auto& s : passives)
	{
		character.maxHp += s.hpBonus;
		character.maxMp += s.mpBonus;
		character.atkBonus += s.atkBonus;
		character.ac += s.acBonus;
		character.damageBonus += s.damageBonus;
	}

	// Clamp
	if (character.hp > character.maxHp) character.hp = character.maxHp;
	if (character.mp > character.maxMp) character.mp = character.maxMp;
}
