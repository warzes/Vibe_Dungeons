#pragma once

#include "engine/game_state.h"

#include "engine/renderer/shader.h"
#include "engine/renderer/mesh.h"
#include "engine/renderer/texture.h"
#include "engine/renderer/material.h"
#include "engine/renderer/camera.h"
#include "engine/renderer/debug_renderer.h"
#include "game/grid_position.h"
#include "game/direction.h"
#include "game/overworld/overworld.h"
#include "game/overworld/overworld_renderer.h"
#include "game/turn_manager.h"
#include "game/combat/character.h"
#include "core/json_alias.h"

class GameStateMachine;
class InputManager;
class Window;
class DeltaTime;
class Renderer;
class ResourceManager;

class OverworldState final : public GameState
{
public:
	OverworldState(
		GameStateMachine& machine,
		const Window& window,
		InputManager& input,
		ResourceManager& resources
	) noexcept;
	~OverworldState() noexcept override;

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
		return "Overworld";
	}

private:
	// Palette texture helpers
	[[nodiscard]] Texture* createTerrainPalette() noexcept;

	// Movement helpers
	void processEdgeActions() noexcept;
	void processHeldRepeat(const DeltaTime& dt) noexcept;
	void processInteraction() noexcept;

	[[nodiscard]] bool isWalkableAction(std::string_view name) const noexcept;
	void doGridAction(std::string_view name) noexcept;

	void renderMinimap() noexcept;
	void renderFullMap() noexcept;

	GameStateMachine& m_machine;
	const Window& m_window;
	InputManager& m_input;
	ResourceManager& m_resources;

	Shader* m_overworldShader = nullptr;
	Texture* m_terrainPalette = nullptr;
	Material* m_floorMaterial = nullptr;
	Material* m_wallMaterial = nullptr;
	Material* m_locationMaterial = nullptr;

	Camera m_camera;
	Overworld m_overworld;
	OverworldRenderer m_overworldRenderer;
	DebugRenderer m_debugRenderer;

	bool m_initialized = false;
	const Renderer* m_renderer = nullptr;

	float m_moveRepeatDelay = 0.1f;
	float m_moveRepeatTimer = 0.0f;

	bool m_showMap = false;
	bool m_showDebug = false;

	// Visibility radius
	int32_t m_viewRadius = 5;

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
