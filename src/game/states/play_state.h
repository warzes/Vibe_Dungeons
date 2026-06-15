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
		ResourceManager& resources
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

	GameStateMachine& m_machine;
	const Window& m_window;
	InputManager& m_input;
	ResourceManager& m_resources;

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

	bool m_initialized = false;
	const Renderer* m_renderer = nullptr;

	float m_moveRepeatDelay = 0.1f;
	float m_moveRepeatTimer = 0.0f;
};
