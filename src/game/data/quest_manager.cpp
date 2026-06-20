#include "stdafx.h"
#include "game/data/quest_manager.h"
#include "core/file_io.h"
#include "core/logger.h"

std::vector<Quest> QuestManager::s_quests;

static QuestObjectiveType parseObjectiveType(const std::string& str)
{
	if (str == "kill")    return QuestObjectiveType::Kill;
	if (str == "collect") return QuestObjectiveType::Collect;
	if (str == "explore") return QuestObjectiveType::Explore;
	if (str == "talk")    return QuestObjectiveType::Talk;
	return QuestObjectiveType::Kill;
}

void QuestManager::LoadQuests(const char* filePath)
{
	Clear();

	std::string text = FileReadString(filePath);
	if (text.empty())
	{
		Logger::Warn(std::string("QuestManager: could not read ") + filePath);
		return;
	}

	json j = json::parse(text, nullptr, false);
	if (j.is_discarded() || !j.contains("quests"))
	{
		Logger::Warn(std::string("QuestManager: invalid JSON in ") + filePath);
		return;
	}

	for (const auto& entry : j["quests"])
	{
		Quest q;
		q.id = entry.value("id", "");
		q.name = entry.value("name", "Unknown Quest");
		q.description = entry.value("description", "");
		q.giverNpcId = entry.value("giverNpcId", "");
		q.nextQuestId = entry.value("nextQuestId", "");
		q.levelReq = entry.value("levelReq", 1);

		if (entry.contains("objectives"))
		{
			for (const auto& obj : entry["objectives"])
			{
				QuestObjective o;
				o.type = parseObjectiveType(obj.value("type", "kill"));
				o.target = obj.value("target", "");
				o.amount = obj.value("amount", 1);
				q.objectives.push_back(std::move(o));
			}
		}

		if (entry.contains("reward"))
		{
			const auto& r = entry["reward"];
			q.reward.xp = r.value("xp", 0);
			q.reward.gold = r.value("gold", 0);
			q.reward.reputation = r.value("reputation", 0);
			if (r.contains("items"))
			{
				for (const auto& it : r["items"])
				{
					q.reward.itemIds.push_back(it.get<std::string>());
				}
			}
		}

		s_quests.push_back(std::move(q));
	}

	Logger::Info(std::string("QuestManager: loaded ") + std::to_string(s_quests.size()) + " quests");
}

void QuestManager::Clear() noexcept
{
	s_quests.clear();
}

const Quest* QuestManager::GetQuest(const std::string& id)
{
	for (const auto& q : s_quests)
	{
		if (q.id == id)
		{
			return &q;
		}
	}
	return nullptr;
}

std::vector<const Quest*> QuestManager::GetQuestsForNpc(const std::string& npcId)
{
	std::vector<const Quest*> result;
	for (const auto& q : s_quests)
	{
		if (q.giverNpcId == npcId)
		{
			result.push_back(&q);
		}
	}
	return result;
}

std::vector<const Quest*> QuestManager::GetQuestsForLevel(int32_t level)
{
	std::vector<const Quest*> result;
	for (const auto& q : s_quests)
	{
		if (q.levelReq <= level)
		{
			result.push_back(&q);
		}
	}
	return result;
}
