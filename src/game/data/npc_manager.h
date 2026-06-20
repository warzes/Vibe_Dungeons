#pragma once

#include <string>
#include <vector>
#include "core/json_alias.h"

enum class NpcType : uint8_t
{
	Merchant,
	Trainer,
	QuestGiver,
	Barkeeper
};

struct NpcDialogue final
{
	std::string text;
	std::vector<std::string> responses;
};

struct Npc final
{
	std::string id;
	std::string name;
	NpcType type = NpcType::Merchant;
	std::string locationId;
	std::vector<NpcDialogue> dialogues;
	std::vector<std::string> itemIds;    // items sold (merchants)
	std::string shopTableId;
	std::vector<std::string> questIds;
	std::string portraitPath;
};

struct NpcManager final
{
	static void LoadNpcs(const char* filePath);
	static void Clear() noexcept;

	[[nodiscard]] static const Npc* GetNpc(const std::string& id);
	[[nodiscard]] static const std::vector<Npc>& AllNpcs() noexcept { return s_npcs; }
	[[nodiscard]] static std::vector<const Npc*> GetNpcsAtLocation(const std::string& locationId);

private:
	static std::vector<Npc> s_npcs;
};
