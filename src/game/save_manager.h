#pragma once

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
}
