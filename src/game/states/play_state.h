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
#include "game/game_mode.h"
#include "game/combat/turn_queue.h"
#include "game/combat/character.h"
#include "game/combat/monster_manager.h"
#include "game/combat/monster_renderer.h"
#include "game/combat/combat_system.h"
#include "game/combat/item_renderer.h"
#include "game/combat/item.h"
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
	void performCombat() noexcept;

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
	GameMode m_gameMode = GameMode::Exploring;
	TurnQueue m_turnQueue;

	bool m_initialized = false;
	const Renderer* m_renderer = nullptr;

	float m_moveRepeatDelay = 0.1f;
	float m_moveRepeatTimer = 0.0f;

	// Combat
	std::unordered_map<std::string, Texture*> m_monsterTextures;
	std::unordered_map<std::string, Material*> m_monsterMaterials;

	Character m_character;
	MonsterManager m_monsterManager;
	MonsterRenderer m_monsterRenderer;
	CombatSystem m_combatSystem;
	CombatLog m_combatLog;

	// Items
	Texture* m_texItemHeal = nullptr;
	Texture* m_texItemMana = nullptr;
	Texture* m_texItemKey = nullptr;
	Texture* m_texItemGold = nullptr;
	Texture* m_texItemScroll = nullptr;
	Texture* m_texItemSword = nullptr;
	Texture* m_texItemArmor = nullptr;
	Texture* m_texItemShield = nullptr;
	Texture* m_texItemQuest = nullptr;
	ItemRenderer m_itemRenderer;
	std::vector<ItemDrop> m_itemDrops;
	bool m_showInventory = false;

	void processPickup() noexcept;
	void renderInventoryWindow() noexcept;
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
	void processActionSlot(int32_t slotIndex) noexcept;
	void applyRegen() noexcept;
	void useAbility(const std::string& abilityId) noexcept;
};
