#pragma once

#include "engine/game_state.h"

#include "engine/renderer/shader.h"
#include "engine/renderer/mesh.h"
#include "engine/renderer/texture.h"
#include "engine/renderer/material.h"
#include "engine/renderer/camera.h"
#include "engine/renderer/debug_renderer.h"
#include "engine/audio_system.h"
#include "core/transform.h"
#include "core/collision.h"
#include "core/profiler.h"

class GameStateMachine;
class InputManager;
class Window;
class DeltaTime;
class Renderer;
class ResourceManager;

struct CubeData
{
	Transform transform;
	glm::vec3 rotationAxis;
	float angle;
	float speed;
	int32_t materialIndex;
	glm::mat4 worldMatrix = glm::mat4(1.0f);
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
	GameStateMachine& m_machine;
	const Window& m_window;
	InputManager& m_input;
	ResourceManager& m_resources;

	Shader* m_shader = nullptr;
	Mesh* m_cube = nullptr;
	Camera m_camera;
	DebugRenderer m_debugRenderer;
	AudioSystem m_audio;

	static constexpr int32_t NUM_MATERIALS = 4;
	Material* m_materials[NUM_MATERIALS]{};

	std::vector<CubeData> m_cubes;
	Sphere m_debugSphere;

	bool m_initialized = false;
	bool m_showDebug = false;
	const Renderer* m_renderer = nullptr;
	int32_t m_collidingCubes = 0;
};
