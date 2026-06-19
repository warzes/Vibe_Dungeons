#pragma once

#include "engine/game_state.h"
#include "engine/renderer/shader.h"
#include "engine/renderer/mesh.h"
#include "engine/renderer/texture.h"
#include "engine/renderer/material.h"
#include "engine/renderer/camera.h"
#include "engine/renderer/debug_renderer.h"
#include "game/combat/character.h"
#include "game/combat/combat_handler.h"
#include "game/combat/combat_system.h"
#include "game/ui/combat_log.h"
#include "game/combat/monster_manager.h"
#include "game/combat/monster_renderer.h"
#include "game/data/monster_factory.h"
#include "game/combat/item_handler.h"
#include "game/combat/spell_system.h"
#include "game/dungeon/dungeon.h"
#include "game/turn_manager.h"
#include "game/grid_position.h"
#include "game/direction.h"

class GameStateMachine;
class InputManager;
class Window;
class DeltaTime;
class Renderer;
class ResourceManager;

// @brief Describes which monsters to spawn for an encounter.
struct EncounterSpawnEntry final
{
	std::string monsterTypeId;
	int32_t count = 1;
	int32_t level = 1;
};

class CombatEncounterState final : public GameState
{
public:
	CombatEncounterState(
		GameStateMachine& machine,
		const Window& window,
		InputManager& input,
		ResourceManager& resources,
		Character* pendingCharacter
	) noexcept;
	~CombatEncounterState() noexcept override;

	// Set this before PushState("CombatEncounter")
	static std::vector<EncounterSpawnEntry> s_spawnEntries;

	void OnEnter() noexcept override;
	void OnExit() noexcept override;
	void OnPause() noexcept override;
	void OnResume() noexcept override;
	void HandleEvent(const SDL_Event& event) noexcept override;
	void FixedUpdate(float fixedDt) noexcept override;
	void Update(const DeltaTime& dt) noexcept override;
	void Render() noexcept override;
	void RenderScene(Renderer& renderer) noexcept override;
	void RenderOverlay(const glm::mat4& viewProj) noexcept override;

	[[nodiscard]] std::string_view GetName() const noexcept override
	{
		return "CombatEncounter";
	}

private:
	void processEdgeActions() noexcept;
	void processHeldRepeat(const DeltaTime& dt) noexcept;
	void processAttack() noexcept;
	void processTurnWaiting() noexcept;
	void doGridAction(std::string_view name) noexcept;
	[[nodiscard]] bool isWalkableAction(std::string_view name) const noexcept;
	void checkEncounterResult() noexcept;
	void createArenaDungeon() noexcept;

	GameStateMachine& m_machine;
	const Window& m_window;
	InputManager& m_input;
	ResourceManager& m_resources;
	Character* m_pendingCharacter;

	Character m_character;
	Camera m_camera;
	Dungeon m_dungeon;
	TurnManager m_turnManager;
	MonsterManager m_monsterManager;
	MonsterRenderer m_monsterRenderer;
	CombatHandler m_combatHandler;
	CombatSystem m_combatSystem;
	CombatLog m_combatLog;
	ItemHandler m_itemHandler;
	SpellSystem m_spellSystem;
	DebugRenderer m_debugRenderer;

	Shader* m_dungeonShader = nullptr;
	Texture* m_texFloor = nullptr;
	Texture* m_texWall = nullptr;
	Texture* m_texCeiling = nullptr;
	Material* m_matFloor = nullptr;
	Material* m_matWall = nullptr;
	Material* m_matCeiling = nullptr;

	bool m_initialized = false;
	bool m_encounterDone = false;
	bool m_pendingLevelUp = false;
	bool m_showDebug = false;
	bool m_showVictory = false;
	bool m_showDefeat = false;

	// Arena layout constants
	static constexpr int32_t ARENA_SIZE = 9;
	static constexpr int32_t ARENA_ORIGIN_ROW = 13;
	static constexpr int32_t ARENA_ORIGIN_COL = 13;
	static constexpr int32_t PLAYER_START_ROW = 15;
	static constexpr int32_t PLAYER_START_COL = 14;

	static constexpr std::string_view GRID_ACTION_NAMES[] =
	{
		"GridMoveForward",
		"GridMoveBackward",
		"TurnLeft",
		"TurnRight",
		"StrafeLeft",
		"StrafeRight"
	};
};
