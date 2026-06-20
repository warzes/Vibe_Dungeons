#include "stdafx.h"
#include "game/combat/character.h"
#include "game/data/item_factory.h"
#include "game/data/quest_manager.h"
#include "core/logger.h"

//=============================================================================

void Character::TakeDamage(int32_t amount) noexcept
{
	m_hp -= amount;
	if (m_hp < 0)
	{
		m_hp = 0;
	}
}

//=============================================================================

void Character::Heal(int32_t amount) noexcept
{
	m_hp += amount;
	if (m_hp > m_maxHp)
	{
		m_hp = m_maxHp;
	}
}

//=============================================================================

void Character::SpendMp(int32_t amount) noexcept
{
	m_mp -= amount;
	if (m_mp < 0)
	{
		m_mp = 0;
	}
}

//=============================================================================

void Character::RestoreMp(int32_t amount) noexcept
{
	m_mp += amount;
	if (m_mp > m_maxMp)
	{
		m_mp = m_maxMp;
	}
}

//=============================================================================

void Character::AcceptQuest(const std::string& questId)
{
	if (HasQuest(questId))
	{
		return;
	}
	const Quest* q = QuestManager::GetQuest(questId);
	if (!q)
	{
		return;
	}
	QuestEntry entry;
	entry.questId = questId;
	entry.status = QuestStatus::Active;
	entry.currentObjective = 0;
	entry.objectiveProgress.resize(q->objectives.size(), 0);
	m_questLog.push_back(std::move(entry));
	Logger::Info(std::string("Accepted quest: ") + questId);
}

//=============================================================================

bool Character::HasQuest(const std::string& questId) const
{
	for (const auto& entry : m_questLog)
	{
		if (entry.questId == questId)
		{
			return true;
		}
	}
	return false;
}

//=============================================================================

bool Character::IsQuestCompleted(const std::string& questId) const
{
	for (const auto& entry : m_questLog)
	{
		if (entry.questId == questId && entry.status == QuestStatus::Completed)
		{
			return true;
		}
	}
	return false;
}

//=============================================================================

void Character::AdvanceQuestObjective(const std::string& questId)
{
	// Find the quest and advance to next objective
	for (auto& entry : m_questLog)
	{
		if (entry.questId != questId || entry.status != QuestStatus::Active)
		{
			continue;
		}
		const Quest* q = QuestManager::GetQuest(questId);
		if (!q)
		{
			continue;
		}
		entry.currentObjective++;
		if (entry.currentObjective >= static_cast<int32_t>(q->objectives.size()))
		{
			entry.status = QuestStatus::Completed;
			// Apply reward
			m_xp += q->reward.xp;
			m_gold += q->reward.gold;
			m_reputation += q->reward.reputation;

			for (const auto& itemId : q->reward.itemIds)
			{
				Item rewardItem = ItemFactory::CreateBase(itemId);
				if (m_inventory.Add(rewardItem) == AddResult::Success)
				{
					Logger::Info(std::string("Quest reward: received ") + itemId);
				}
			}
			Logger::Info(std::string("Quest completed: ") + questId);

			// Chain to next quest if any
			if (!q->nextQuestId.empty())
			{
				AcceptQuest(q->nextQuestId);
			}
		}
	}
}

//=============================================================================

void Character::CompleteQuestObjective(const std::string& questId, int32_t objectiveIdx)
{
	for (auto& entry : m_questLog)
	{
		if (entry.questId != questId || entry.status != QuestStatus::Active)
		{
			continue;
		}
		const Quest* q = QuestManager::GetQuest(questId);
		if (!q || objectiveIdx < 0 || objectiveIdx >= static_cast<int32_t>(q->objectives.size()))
		{
			continue;
		}
		entry.objectiveProgress[objectiveIdx] = q->objectives[objectiveIdx].amount;
		AdvanceQuestObjective(questId);
	}
}

//=============================================================================

void Character::TickQuestKill(const std::string& monsterId)
{
	for (auto& entry : m_questLog)
	{
		if (entry.status != QuestStatus::Active)
		{
			continue;
		}
		const Quest* q = QuestManager::GetQuest(entry.questId);
		if (!q)
		{
			continue;
		}
		int32_t idx = 0;
		bool completedAny = false;
		for (const auto& obj : q->objectives)
		{
			if (obj.type == QuestObjectiveType::Kill && obj.target == monsterId)
			{
				entry.objectiveProgress[idx]++;
				if (entry.objectiveProgress[idx] >= obj.amount)
				{
					entry.objectiveProgress[idx] = obj.amount;
					completedAny = true;
				}
			}
			idx++;
		}
		if (completedAny)
		{
			AdvanceQuestObjective(entry.questId);
		}
	}
}

//=============================================================================

void Character::TickQuestCollect(const std::string& itemId)
{
	for (auto& entry : m_questLog)
	{
		if (entry.status != QuestStatus::Active)
		{
			continue;
		}
		const Quest* q = QuestManager::GetQuest(entry.questId);
		if (!q)
		{
			continue;
		}
		int32_t idx = 0;
		bool completedAny = false;
		for (const auto& obj : q->objectives)
		{
			if (obj.type == QuestObjectiveType::Collect && obj.target == itemId)
			{
				entry.objectiveProgress[idx]++;
				if (entry.objectiveProgress[idx] >= obj.amount)
				{
					entry.objectiveProgress[idx] = obj.amount;
					completedAny = true;
				}
			}
			idx++;
		}
		if (completedAny)
		{
			AdvanceQuestObjective(entry.questId);
		}
	}
}

//=============================================================================

void Character::TickQuestExplore(const std::string& locationId)
{
	for (auto& entry : m_questLog)
	{
		if (entry.status != QuestStatus::Active)
		{
			continue;
		}
		const Quest* q = QuestManager::GetQuest(entry.questId);
		if (!q)
		{
			continue;
		}
		int32_t idx = 0;
		bool completedAny = false;
		for (const auto& obj : q->objectives)
		{
			if (obj.type == QuestObjectiveType::Explore && obj.target == locationId)
			{
				entry.objectiveProgress[idx]++;
				if (entry.objectiveProgress[idx] >= obj.amount)
				{
					entry.objectiveProgress[idx] = obj.amount;
					completedAny = true;
				}
			}
			idx++;
		}
		if (completedAny)
		{
			AdvanceQuestObjective(entry.questId);
		}
	}
}

//=============================================================================

void Character::TickQuestTalk(const std::string& npcId)
{
	for (auto& entry : m_questLog)
	{
		if (entry.status != QuestStatus::Active)
		{
			continue;
		}
		const Quest* q = QuestManager::GetQuest(entry.questId);
		if (!q)
		{
			continue;
		}
		int32_t idx = 0;
		bool completedAny = false;
		for (const auto& obj : q->objectives)
		{
			if (obj.type == QuestObjectiveType::Talk && obj.target == npcId)
			{
				entry.objectiveProgress[idx]++;
				if (entry.objectiveProgress[idx] >= obj.amount)
				{
					entry.objectiveProgress[idx] = obj.amount;
					completedAny = true;
				}
			}
			idx++;
		}
		if (completedAny)
		{
			AdvanceQuestObjective(entry.questId);
		}
	}
}
