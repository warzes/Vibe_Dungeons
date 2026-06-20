#pragma once

#include <string>
#include <vector>
#include "core/json_alias.h"

enum class QuestObjectiveType : uint8_t
{
	Kill,
	Collect,
	Explore,
	Talk
};

struct QuestObjective final
{
	QuestObjectiveType type = QuestObjectiveType::Kill;
	std::string target;   // monsterId / itemId / locationId / npcId
	int32_t amount = 1;
	int32_t current = 0;
};

struct QuestReward final
{
	int32_t xp = 0;
	int32_t gold = 0;
	std::vector<std::string> itemIds;
	int32_t reputation = 0;
};

struct Quest final
{
	std::string id;
	std::string name;
	std::string description;
	std::vector<QuestObjective> objectives;
	QuestReward reward;
	std::string giverNpcId;    // which NPC gives this quest
	std::string nextQuestId;   // chain: next quest after this one
	int32_t levelReq = 1;      // minimum character level
};

enum class QuestStatus : uint8_t
{
	Inactive,
	Active,
	Completed,
	Failed
};

struct QuestEntry final
{
	std::string questId;
	QuestStatus status = QuestStatus::Inactive;
	int32_t currentObjective = 0;
	std::vector<int32_t> objectiveProgress; // per-objective progress
};

struct QuestManager final
{
	static void LoadQuests(const char* filePath);
	static void Clear() noexcept;

	[[nodiscard]] static const Quest* GetQuest(const std::string& id);
	[[nodiscard]] static const std::vector<Quest>& AllQuests() noexcept { return s_quests; }
	[[nodiscard]] static std::vector<const Quest*> GetQuestsForNpc(const std::string& npcId);
	[[nodiscard]] static std::vector<const Quest*> GetQuestsForLevel(int32_t level);

private:
	static std::vector<Quest> s_quests;
};
