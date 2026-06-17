#pragma once

#include "engine/game_state.h"
#include "engine/delta_time.h"
#include "engine/input_manager.h"
#include "engine/resource_manager.h"
#include "engine/renderer/framebuffer.h"
#include "engine/renderer/shader.h"
#include "engine/renderer/renderer.h"
#include "game/combat/character.h"

class Window;
class GLContext;

class GameApp final
{
public:
	GameApp();
	~GameApp() noexcept;

	GameApp(const GameApp&) = delete;
	GameApp& operator=(const GameApp&) = delete;

	void Run();
	void Frame();

	[[nodiscard]] bool IsRunning() const noexcept
	{
		return m_running;
	}

private:
	void processEvents();
	void update();
	void render();
	void initImGui();
	void shutdownImGui() noexcept;
	void initScreenQuad();
	void destroyScreenQuad() noexcept;
	void drawScreenQuad() noexcept;

	std::unique_ptr<Window> m_window;
	std::unique_ptr<GLContext> m_glContext;
	GameStateMachine m_stateMachine;
	InputManager m_input;
	DeltaTime m_deltaTime;
	ResourceManager m_resources;
	Character m_pendingCharacter;
	int32_t m_renderHeight;
	FrameBuffer m_fbo;
	Renderer m_renderer;
	Shader m_screenShader;
	uint32_t m_screenQuadVAO = 0;
	uint32_t m_screenQuadVBO = 0;

	bool m_running = true;

	static constexpr double TARGET_FRAME_TIME = 1.0 / 120.0;
	static constexpr double SLEEP_THRESHOLD = 0.002;
	static constexpr double PHYSICS_DT = 1.0 / 60.0;

	double m_accumulator = 0.0;

};