#include "stdafx.h"
#include "game/save_manager.h"
#include "game/combat/character.h"
#include "game/dungeon/dungeon.h"
#include "game/combat/monster_manager.h"
#include "engine/renderer/camera.h"
#include "game/dungeon/dungeon_renderer.h"
#include "game/ui/combat_log.h"
#include "game/serialization.h"
#include "core/file_io.h"

#include <cstdio>
#include <format>

//=============================================================================

static std::string SlotPath(int32_t slot)
{
	return std::format("save_slot_{}.json", slot);
}

//=============================================================================

bool SaveManager::SaveGame(
	const char* path,
	const Character& character,
	const Dungeon& dungeon,
	const MonsterManager& monsterManager,
	const std::vector<ItemDrop>& itemDrops,
	CombatLog& combatLog
) noexcept
{
	try
	{
		json j;

		// Character
		j["character"] = character;

		// Dungeon
		json dungeonJson;
		to_json(dungeonJson, dungeon.GetChunk());
		j["dungeon"] = dungeonJson;

		// Monsters
		json monstersJson = json::array();
		for (const Monster& m : monsterManager.All())
		{
			monstersJson.push_back(m);
		}
		j["monsters"] = monstersJson;

		// Item drops
		j["itemDrops"] = itemDrops;

		std::string dumped = j.dump(2);

		if (!FileWriteBytes(path, dumped.data(), dumped.size()))
		{
			combatLog.Add("Save failed: cannot open file.", glm::vec3(1.0f, 0.3f, 0.0f));
			return false;
		}

		combatLog.Add("Game saved.", glm::vec3(0.3f, 0.8f, 1.0f));
		return true;
	}
	catch (const std::exception& e)
	{
		combatLog.Add("Save error: " + std::string(e.what()), glm::vec3(1.0f, 0.3f, 0.0f));
		return false;
	}
}

//=============================================================================

bool SaveManager::LoadGame(
	const char* path,
	Character& character,
	Dungeon& dungeon,
	MonsterManager& monsterManager,
	std::vector<ItemDrop>& itemDrops,
	Camera& camera,
	DungeonRenderer& dungeonRenderer,
	CombatLog& combatLog
) noexcept
{
	try
	{
		std::string buffer = FileReadString(path);
		if (buffer.empty())
		{
			combatLog.Add("Load failed: file not found.", glm::vec3(1.0f, 0.3f, 0.0f));
			return false;
		}

		json j = json::parse(buffer);

		// Validate required fields (step 357)
		if (!j.contains("character") || !j.contains("dungeon") || !j.contains("monsters") || !j.contains("itemDrops"))
		{
			combatLog.Add("Load failed: save file missing required fields.", glm::vec3(1.0f, 0.3f, 0.0f));
			return false;
		}

		// Character
		character = j.at("character").get<Character>();

		// Synchronize camera with loaded character position
		camera.SetGridPosition(character.GetPosition(), character.GetFacing());
		camera.SnapToGrid();

		// Dungeon
		Chunk loadedChunk;
		from_json(j.at("dungeon"), loadedChunk);
		dungeon.SetChunk(loadedChunk);
		dungeonRenderer.SetNeedsRebuild(true);

		// Monsters
		monsterManager.Clear();
		for (const auto& monsterJson : j.at("monsters"))
		{
			monsterManager.Spawn(monsterJson.get<Monster>());
		}

		// Item drops
		itemDrops = j.at("itemDrops").get<std::vector<ItemDrop>>();

		combatLog.Add("Game loaded.", glm::vec3(0.3f, 0.8f, 1.0f));
		return true;
	}
	catch (const std::exception& e)
	{
		combatLog.Add("Load error: " + std::string(e.what()), glm::vec3(1.0f, 0.3f, 0.0f));
		return false;
	}
}

//=============================================================================

bool SaveManager::SaveGameToSlot(
	int32_t slot,
	const Character& character,
	const Dungeon& dungeon,
	const MonsterManager& monsterManager,
	const std::vector<ItemDrop>& itemDrops,
	CombatLog& combatLog
) noexcept
{
	if (slot < 0 || slot >= NUM_SAVE_SLOTS)
	{
		combatLog.Add("Save failed: invalid slot.", glm::vec3(1.0f, 0.3f, 0.0f));
		return false;
	}
	return SaveGame(SlotPath(slot).c_str(), character, dungeon, monsterManager, itemDrops, combatLog);
}

//=============================================================================

bool SaveManager::LoadGameFromSlot(
	int32_t slot,
	Character& character,
	Dungeon& dungeon,
	MonsterManager& monsterManager,
	std::vector<ItemDrop>& itemDrops,
	Camera& camera,
	DungeonRenderer& dungeonRenderer,
	CombatLog& combatLog
) noexcept
{
	if (slot < 0 || slot >= NUM_SAVE_SLOTS)
	{
		combatLog.Add("Load failed: invalid slot.", glm::vec3(1.0f, 0.3f, 0.0f));
		return false;
	}
	return LoadGame(SlotPath(slot).c_str(), character, dungeon, monsterManager, itemDrops, camera, dungeonRenderer, combatLog);
}

//=============================================================================

bool SaveManager::SlotExists(int32_t slot) noexcept
{
	if (slot < 0 || slot >= NUM_SAVE_SLOTS)
	{
		return false;
	}

	FILE* f = fopen(SlotPath(slot).c_str(), "rb");
	if (!f)
	{
		return false;
	}
	fclose(f);
	return true;
}

//=============================================================================

std::string SaveManager::GetSlotInfo(int32_t slot) noexcept
{
	if (slot < 0 || slot >= NUM_SAVE_SLOTS)
	{
		return "Invalid slot";
	}

	try
	{
		std::string buffer = FileReadString(SlotPath(slot).c_str());
		if (buffer.empty())
		{
			return "Empty";
		}

		json j = json::parse(buffer);
		const json& chr = j.at("character");

		std::string name = chr.at("name").get<std::string>();
		int32_t level = chr.at("level").get<int32_t>();

		return std::format("{} (Lvl {})", name, level);
	}
	catch (...)
	{
		return "Corrupted";
	}
}
