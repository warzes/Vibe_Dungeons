#include "stdafx.h"
#include "game/data/npc_manager.h"
#include "core/file_io.h"
#include "core/logger.h"

std::vector<Npc> NpcManager::s_npcs;

static NpcType parseNpcType(const std::string& str)
{
	if (str == "merchant")    return NpcType::Merchant;
	if (str == "trainer")     return NpcType::Trainer;
	if (str == "quest_giver") return NpcType::QuestGiver;
	if (str == "barkeeper")   return NpcType::Barkeeper;
	return NpcType::Merchant;
}

void NpcManager::LoadNpcs(const char* filePath)
{
	Clear();

	std::string text = FileReadString(filePath);
	if (text.empty())
	{
		Logger::Warn(std::string("NpcManager: could not read ") + filePath);
		return;
	}

	json j = json::parse(text, nullptr, false);
	if (j.is_discarded() || !j.contains("npcs"))
	{
		Logger::Warn(std::string("NpcManager: invalid JSON in ") + filePath);
		return;
	}

	for (const auto& entry : j["npcs"])
	{
		Npc npc;
		npc.id = entry.value("id", "");
		npc.name = entry.value("name", "Unknown");
		npc.type = parseNpcType(entry.value("type", "merchant"));
		npc.locationId = entry.value("locationId", "");
		npc.shopTableId = entry.value("shopTableId", "");
		npc.portraitPath = entry.value("portrait", "");

		if (entry.contains("dialogues"))
		{
			for (const auto& d : entry["dialogues"])
			{
				NpcDialogue dia;
				dia.text = d.value("text", "");
				if (d.contains("responses"))
				{
					for (const auto& r : d["responses"])
					{
						dia.responses.push_back(r.get<std::string>());
					}
				}
				npc.dialogues.push_back(std::move(dia));
			}
		}

		if (entry.contains("items"))
		{
			for (const auto& it : entry["items"])
			{
				npc.itemIds.push_back(it.get<std::string>());
			}
		}

		if (entry.contains("questIds"))
		{
			for (const auto& q : entry["questIds"])
			{
				npc.questIds.push_back(q.get<std::string>());
			}
		}

		s_npcs.push_back(std::move(npc));
	}

	Logger::Info(std::string("NpcManager: loaded ") + std::to_string(s_npcs.size()) + " NPCs");
}

void NpcManager::Clear() noexcept
{
	s_npcs.clear();
}

const Npc* NpcManager::GetNpc(const std::string& id)
{
	for (const auto& npc : s_npcs)
	{
		if (npc.id == id)
		{
			return &npc;
		}
	}
	return nullptr;
}

std::vector<const Npc*> NpcManager::GetNpcsAtLocation(const std::string& locationId)
{
	std::vector<const Npc*> result;
	for (const auto& npc : s_npcs)
	{
		if (npc.locationId == locationId)
		{
			result.push_back(&npc);
		}
	}
	return result;
}
