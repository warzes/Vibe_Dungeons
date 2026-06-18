#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "game/combat/status_effect.h"
#include "game/combat/monster_group.h"

struct Skill;
struct Monster;
class Character;
class MonsterManager;
class CombatSystem;
class CombatLog;
class TurnManager;
class Camera;
class Dungeon;
class ResourceManager;
class Shader;
class Renderer;
class MonsterRenderer;
class ItemHandler;
struct Material;
struct Texture;

class CombatHandler final
{
public:
	void Init(
		Character& character,
		MonsterManager& monsterManager,
		MonsterRenderer& monsterRenderer,
		CombatSystem& combatSystem,
		CombatLog& combatLog,
		TurnManager& turnManager,
		Camera& camera,
		Dungeon& dungeon,
		ResourceManager& resources,
		Shader& dungeonShader,
		bool& pendingLevelUp,
		ItemHandler& itemHandler
	) noexcept;

	void LoadMonsterTextures() noexcept;
	void SpawnDefault() noexcept;

	void ProcessAttack() noexcept;
	void PerformCombat() noexcept;
	void UseAbility(const std::string& abilityId) noexcept;
	void ProcessActionSlot(int32_t slotIndex) noexcept;

	// Ranged combat (step 133)
	void PerformRangedAttack() noexcept;

	// Monster AI combat (steps 137, 160)
	void ProcessMonsterTurns() noexcept;

	// Check if weapon is ranged
	[[nodiscard]] bool HasRangedWeaponEquipped() const noexcept;
	[[nodiscard]] int32_t GetEquippedWeaponRange() const noexcept;

	// Status effects system
	[[nodiscard]] StatusSystem& GetStatusSystem() noexcept { return m_statusSystem; }

	void SubmitRender(Renderer& renderer) noexcept;

private:
	void useMeleeAbility(const Skill& skill) noexcept;
	void useMovementAbility(const Skill& skill) noexcept;
	void useRangedAbility(const Skill& skill) noexcept;
	void useSelfAbility(const Skill& skill) noexcept;
	void onKill(Monster& target) noexcept;
	void reduceWeaponDurability() noexcept;
	bool hasAmmo() const noexcept;
	void consumeAmmo() noexcept;

	Character* m_character = nullptr;
	MonsterManager* m_monsterManager = nullptr;
	MonsterRenderer* m_monsterRenderer = nullptr;
	CombatSystem* m_combatSystem = nullptr;
	CombatLog* m_combatLog = nullptr;
	TurnManager* m_turnManager = nullptr;
	Camera* m_camera = nullptr;
	Dungeon* m_dungeon = nullptr;
	ResourceManager* m_resources = nullptr;
	Shader* m_dungeonShader = nullptr;
	bool* m_pendingLevelUp = nullptr;
	ItemHandler* m_itemHandler = nullptr;

	StatusSystem m_statusSystem;

	std::unordered_map<std::string, Texture*> m_monsterTextures;
	std::unordered_map<std::string, Material*> m_monsterMaterials;
};
