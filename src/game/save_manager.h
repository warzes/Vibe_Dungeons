#pragma once

#include <cstdint>
#include <string>
#include <vector>

class Character;
class Dungeon;
class MonsterManager;
class Camera;
class DungeonRenderer;
class CombatLog;
struct ItemDrop;

// @brief Pure save/load functions — no persistent state.
namespace SaveManager
{
	constexpr int32_t NUM_SAVE_SLOTS = 3;

	bool SaveGame(
		const char* path,
		const Character& character,
		const Dungeon& dungeon,
		const MonsterManager& monsterManager,
		const std::vector<ItemDrop>& itemDrops,
		CombatLog& combatLog
	) noexcept;

	bool LoadGame(
		const char* path,
		Character& character,
		Dungeon& dungeon,
		MonsterManager& monsterManager,
		std::vector<ItemDrop>& itemDrops,
		Camera& camera,
		DungeonRenderer& dungeonRenderer,
		CombatLog& combatLog
	) noexcept;

	bool SaveGameToSlot(
		int32_t slot,
		const Character& character,
		const Dungeon& dungeon,
		const MonsterManager& monsterManager,
		const std::vector<ItemDrop>& itemDrops,
		CombatLog& combatLog
	) noexcept;

	bool LoadGameFromSlot(
		int32_t slot,
		Character& character,
		Dungeon& dungeon,
		MonsterManager& monsterManager,
		std::vector<ItemDrop>& itemDrops,
		Camera& camera,
		DungeonRenderer& dungeonRenderer,
		CombatLog& combatLog
	) noexcept;

	bool SlotExists(int32_t slot) noexcept;
	std::string GetSlotInfo(int32_t slot) noexcept;
}
