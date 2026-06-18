#pragma once

#include "engine/game_state.h"

#include "engine/renderer/shader.h"
#include "engine/renderer/mesh.h"
#include "engine/renderer/texture.h"
#include "engine/renderer/material.h"
#include "engine/renderer/camera.h"
#include "engine/renderer/debug_renderer.h"
#include "engine/audio_system.h"
#include "core/profiler.h"
#include "game/grid_position.h"
#include "game/direction.h"
#include "game/dungeon/dungeon.h"
#include "game/dungeon/dungeon_renderer.h"
#include "game/turn_manager.h"
#include "game/combat/character.h"
#include "game/combat/monster_manager.h"
#include "game/combat/monster_renderer.h"
#include "game/combat/combat_handler.h"
#include "game/combat/combat_system.h"
#include "game/combat/spell_system.h"
#include "game/combat/item_handler.h"
#include "game/combat/status_effect.h"
#include "game/data/encounter_manager.h"
#include "game/crafting/crafting_system.h"
#include "game/ui/combat_log.h"

class GameStateMachine;
class InputManager;
class Window;
class DeltaTime;
class Renderer;
class ResourceManager;

struct SavedCameraState
{
	glm::vec3 position;
	float yaw;
	float pitch;
	GridPosition gridPos;
	Direction facing = Direction::North;
	bool isAnimating = false;
};

class PlayState final : public GameState
{
public:
	PlayState(
		GameStateMachine& machine,
		const Window& window,
		InputManager& input,
		ResourceManager& resources,
		Character* pendingCharacter
	) noexcept;
	~PlayState() noexcept override;

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
		return "Play";
	}

private:
	void enterDebugMode() noexcept;
	void exitDebugMode() noexcept;

	void processEdgeActions() noexcept;
	void processHeldRepeat(const DeltaTime& dt) noexcept;
	void processTurnWaiting() noexcept;
	[[nodiscard]] bool isMovementAction(std::string_view name) const noexcept;
	[[nodiscard]] bool isWalkableAction(std::string_view name) const noexcept;
	void doGridAction(std::string_view name) noexcept;

	void processAttack() noexcept;

	GameStateMachine& m_machine;
	const Window& m_window;
	InputManager& m_input;
	ResourceManager& m_resources;
	Character* m_pendingCharacter = nullptr;

	Shader* m_dungeonShader = nullptr;
	Texture* m_texFloor = nullptr;
	Texture* m_texWall = nullptr;
	Texture* m_texCeiling = nullptr;
	Material* m_matFloor = nullptr;
	Material* m_matWall = nullptr;
	Material* m_matCeiling = nullptr;

	Camera m_camera;
	Dungeon m_dungeon;
	DungeonRenderer m_dungeonRenderer;
	DebugRenderer m_debugRenderer;
	AudioSystem m_audio;

	// Debug camera
	SavedCameraState m_savedCamera;
	bool m_showDebug = false;

	// Turn system
	TurnManager m_turnManager;

	bool m_initialized = false;
	const Renderer* m_renderer = nullptr;

	float m_moveRepeatDelay = 0.1f;
	float m_moveRepeatTimer = 0.0f;
	int32_t m_renderHeight = 480;

	// Combat
	CombatHandler m_combatHandler;

	Character m_character;
	MonsterManager m_monsterManager;
	MonsterRenderer m_monsterRenderer;
	CombatSystem m_combatSystem;
	CombatLog m_combatLog;

	// Items
	ItemHandler m_itemHandler;
	bool m_showInventory = false;

	void renderInventoryWindow() noexcept;

	static constexpr std::string_view GRID_ACTION_NAMES[] =
	{
		"GridMoveForward",
		"GridMoveBackward",
		"TurnLeft",
		"TurnRight",
		"StrafeLeft",
		"StrafeRight"
	};
	void renderMapWindow() noexcept;
	void renderOptionsWindow() noexcept;

	void SaveGame(const char* path) noexcept;
	void LoadGame(const char* path) noexcept;
	void RestartGame() noexcept;

	bool m_showMap = false;
	bool m_showOptions = false;

	// Level-up
	bool m_pendingLevelUp = false;
	bool m_showLevelUp = false;

	// Skills & hotbar
	bool m_showSkills = false;

	void renderHotbar() noexcept;
	void renderSkillsWindow() noexcept;

	// Equipment
	bool m_showEquipment = false;
	void renderEquipmentWindow() noexcept;

	// Spellbook
	bool m_showSpellbook = false;
	void renderSpellbookWindow() noexcept;

	// Spell targeting
	bool m_targetingActive = false;
	std::string m_targetingSpellId;
	std::string m_wandItemSpellId;
	void ProcessSpellAction() noexcept;

	// Spell system
	SpellSystem m_spellSystem;

	// Encounter manager (step 165)
	EncounterManager m_encounterManager;

	// Crafting system (steps 166-173)
	CraftingSystem m_craftingSystem;
	bool m_showCrafting = false;
	void renderCraftingWindow() noexcept;
	void renderWeaponsmithOperations() noexcept;
	void renderArmorsmithOperations() noexcept;
	void renderAlchemyOperations() noexcept;
	void renderCookingOperations() noexcept;

	// Hunger system (step 211)
	int32_t m_hungerTurns = 0;

	// Status effects UI
	void renderStatusEffectsWindow() noexcept;
	void renderHungerIndicator() noexcept;

	// Search action (step 291)
	void processSearch() noexcept;

	// Gathering (steps 205-210)
	[[nodiscard]] bool processGather() noexcept;
	bool m_showStatusEffects = false;

	// Targeting mode for AoE (step 140)
	bool m_targetingModeActive = false;
	TargetingMode m_currentTargetingMode = TargetingMode::Single;
	void renderTargetingOverlay() noexcept;

	// AoE targeting preview (step 141)
	SpellTarget m_targetPreview;
	bool m_showTargetingPreview = false;
	void updateTargetingPreview() noexcept;
	void renderAoeGridOverlay() noexcept;
};
