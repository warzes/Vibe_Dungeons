#include "stdafx.h"
#include "game/game_app.h"
#include "game/states/main_menu_state.h"
#include "game/states/class_selection_state.h"
#include "game/states/play_state.h"
#include "game/states/settings_state.h"
#include "engine/window.h"
#include "engine/gl_context.h"
#include "engine/input_manager.h"
#include "engine/delta_time.h"
#include "core/exception.h"
#include "core/logger.h"
#include "core/profiler.h"
#include "engine/renderer/shader_sources.h"
#include <thread>

//=============================================================================

GameApp::GameApp()
	: m_window(std::make_unique<Window>("Dungeon Crawlers", 1280, 720))
	, m_glContext(std::make_unique<GLContext>(m_window->Handle()))
	, m_input(m_window->Handle())
	, m_renderHeight(GetRenderHeightFromConfig())
	, m_fbo(
		static_cast<uint32_t>(static_cast<float>(m_renderHeight) * static_cast<float>(m_window->Width()) / static_cast<float>(m_window->Height())),
		static_cast<uint32_t>(m_renderHeight)
	)
{
	initScreenQuad();

	Logger::Info("Starting Dungeon Crawlers...");

	initImGui();

	m_stateMachine.RegisterState("MainMenu", [this]()
	{
		return std::make_unique<MainMenuState>(m_stateMachine, *m_window, m_input);
	});
	m_stateMachine.RegisterState("ClassSelection", [this]()
	{
		return std::make_unique<ClassSelectionState>(m_stateMachine, *m_window, m_input, m_pendingCharacter);
	});
	m_stateMachine.RegisterState("Play", [this]()
	{
		return std::make_unique<PlayState>(m_stateMachine, *m_window, m_input, m_resources, &m_pendingCharacter);
	});
	m_stateMachine.RegisterState("Settings", [this]()
	{
		return std::make_unique<SettingsState>(m_stateMachine);
	});

	m_stateMachine.PushState("MainMenu");
	m_stateMachine.ProcessActions();
}

//=============================================================================

GameApp::~GameApp() noexcept
{
	destroyScreenQuad();
	shutdownImGui();
	Logger::Info("Dungeon Crawlers shut down");
}

//=============================================================================

#if defined(__EMSCRIPTEN__)

static void emMainLoop(void* arg)
{
	GameApp& app = *static_cast<GameApp*>(arg);
	app.Frame();
	if (!app.IsRunning())
	{
		emscripten_cancel_main_loop();
	}
}

#endif
//=============================================================================
void GameApp::Run()
{
	Logger::Info("Entering main loop");

#if defined(__EMSCRIPTEN__)
	emscripten_set_main_loop_arg(emMainLoop, this, 0, 1);
#else
	while (m_running)
	{
		Frame();
	}
#endif
}
//=============================================================================
void GameApp::Frame()
{
	m_deltaTime.Tick();
	processEvents();
	if (!m_running) return;
	try
	{
		update();
	}
	catch (const std::exception& e)
	{
		Logger::Fatal(std::string("Unhandled exception in update: ") + e.what());
		m_running = false;
		return;
	}
	catch (...)
	{
		Logger::Fatal("Unhandled unknown exception in update");
		m_running = false;
		return;
	}
	render();

	if constexpr (TARGET_FRAME_TIME > 0.0)
	{
		const double frameTime = m_deltaTime.Seconds();
		const double remaining = TARGET_FRAME_TIME - frameTime;
		if (remaining > SLEEP_THRESHOLD)
		{
#if !defined(__EMSCRIPTEN__)
			std::this_thread::sleep_for(
				std::chrono::duration<double>(remaining)
			);
#endif
		}
	}
}

//=============================================================================

void GameApp::processEvents()
{
	m_input.BeginFrame();

	SDL_Event event;
	while (SDL_PollEvent(&event))
	{
		ImGui_ImplSDL3_ProcessEvent(&event);

		// Block mouse events when ImGui has mouse capture,
		// but always allow right-click for mouse capture toggle.
		// Keyboard events (WASD etc.) always pass through.
		if (ImGui::GetIO().WantCaptureMouse)
		{
			switch (event.type)
			{
				case SDL_EVENT_MOUSE_BUTTON_DOWN:
				case SDL_EVENT_MOUSE_BUTTON_UP:
					if (event.button.button == SDL_BUTTON_RIGHT)
					{
						break;
					}
					[[fallthrough]];
				case SDL_EVENT_MOUSE_MOTION:
				case SDL_EVENT_MOUSE_WHEEL:
					continue;
				default:
					break;
			}
		}

		switch (event.type)
		{
			case SDL_EVENT_QUIT:
				m_running = false;
				return;

			case SDL_EVENT_KEY_DOWN:
				if (event.key.scancode == SDL_SCANCODE_ESCAPE)
				{
					if (m_input.IsMouseCaptured())
					{
						m_input.SetMouseCaptured(false);
					}
					else
					{
						m_running = false;
						return;
					}
				}
				break;

			case SDL_EVENT_MOUSE_BUTTON_DOWN:
				if (event.button.button == SDL_BUTTON_RIGHT)
				{
					m_input.SetMouseCaptured(true);
				}
				break;

			case SDL_EVENT_MOUSE_BUTTON_UP:
				if (event.button.button == SDL_BUTTON_RIGHT)
				{
					m_input.SetMouseCaptured(false);
				}
				break;

			case SDL_EVENT_WINDOW_RESIZED:
			{
				int32_t w = event.window.data1;
				int32_t h = event.window.data2;
				if (w < 1 || h < 1)
				{
					break;
				}
				m_window->OnResized(w, h);
				float aspect = static_cast<float>(w) / static_cast<float>(h);
				m_fbo.Resize(
					static_cast<uint32_t>(static_cast<float>(m_renderHeight) * aspect),
					static_cast<uint32_t>(m_renderHeight)
				);
				break;
			}
		}

		m_input.ProcessEvent(event);
		m_stateMachine.HandleEvent(event);
	}
}

//=============================================================================

void GameApp::update()
{
	// Fixed timestep accumulator
	m_accumulator += m_deltaTime.Seconds();
	while (m_accumulator >= PHYSICS_DT)
	{
		m_stateMachine.FixedUpdate(static_cast<float>(PHYSICS_DT));
		m_stateMachine.ProcessActions();
		m_accumulator -= PHYSICS_DT;

		if (m_stateMachine.IsEmpty())
		{
			m_running = false;
			return;
		}
	}

	// Variable timestep update
	m_stateMachine.Update(m_deltaTime);
	m_stateMachine.ProcessActions();

	if (m_stateMachine.IsEmpty())
	{
		m_running = false;
	}
}

//=============================================================================

void GameApp::render()
{
	Profiler::Get().BeginFrame();

	// ---- FBO Pass: 3D scene into framebuffer ----
	m_fbo.Bind();
	glViewport(0, 0, m_fbo.Width(), m_fbo.Height());
	glEnable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_stateMachine.RenderScene(m_renderer);

	// ---- Screen Pass: FBO texture to window at full resolution ----
	m_fbo.Unbind();
	glViewport(0, 0, m_window->Width(), m_window->Height());
	glDisable(GL_DEPTH_TEST);
	glClear(GL_COLOR_BUFFER_BIT);
	drawScreenQuad();

	// Overlay pass (debug rendering, etc.) at screen resolution, on top of FBO
	m_stateMachine.RenderOverlay(m_renderer.ViewProj());

	// ---- ImGui Pass ----
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL3_NewFrame();
	ImGui::NewFrame();
	m_stateMachine.Render();

	// Profiler window
	{
		PROFILE_SCOPE("ProfilerWindow");
		ImGui::Begin("Profiler", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
		ImGui::Text("Frame: %d", Profiler::Get().FrameCount());
		ImGui::Text("Frame time: %.3f ms", Profiler::Get().FrameTime());
		ImGui::Separator();

		if (ImGui::Button("Reset"))
		{
			Profiler::Get().Reset();
		}

		ImGui::SameLine();

		static bool autoScroll = true;
		ImGui::Checkbox("Auto-scroll", &autoScroll);

		ImGui::Separator();
		ImGui::Columns(5, "ProfilerColumns");
		ImGui::Text("Name");
		ImGui::NextColumn();
		ImGui::Text("Total (ms)");
		ImGui::NextColumn();
		ImGui::Text("Avg (ms)");
		ImGui::NextColumn();
		ImGui::Text("Min (ms)");
		ImGui::NextColumn();
		ImGui::Text("Max (ms)");
		ImGui::Separator();

		for (const auto& sample : Profiler::Get().Samples())
		{
			ImGui::Text("%s", sample.name.c_str());
			ImGui::NextColumn();
			ImGui::Text("%.3f", sample.totalTime);
			ImGui::NextColumn();
			ImGui::Text("%.3f", sample.Average());
			ImGui::NextColumn();
			ImGui::Text("%.3f", sample.minTime);
			ImGui::NextColumn();
			ImGui::Text("%.3f", sample.maxTime);
		}
		ImGui::Columns(1);
		ImGui::End();
	}

	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	m_window->SwapBuffers();

	Profiler::Get().EndFrame();
}

//=============================================================================

void GameApp::initImGui()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui::StyleColorsDark();

	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui_ImplSDL3_InitForOpenGL(m_window->Handle(), m_glContext->Handle());
#if defined(__EMSCRIPTEN__)
	ImGui_ImplOpenGL3_Init("#version 300 es");
#else
	ImGui_ImplOpenGL3_Init("#version 330 core");
#endif

	Logger::Info("ImGui initialized");
}

//=============================================================================

void GameApp::shutdownImGui() noexcept
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL3_Shutdown();
	ImGui::DestroyContext();
}

//=============================================================================
// Screen quad for FBO blit
//=============================================================================

void GameApp::initScreenQuad()
{
	const float vertices[] = {
		// pos            tex
		-1.0f, -1.0f,  0.0f, 0.0f,
		 1.0f, -1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f,  0.0f, 1.0f,
		 1.0f,  1.0f,  1.0f, 1.0f,
	};

	glGenVertexArrays(1, &m_screenQuadVAO);
	glGenBuffers(1, &m_screenQuadVBO);

	glBindVertexArray(m_screenQuadVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_screenQuadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), nullptr);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), reinterpret_cast<const void*>(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	m_screenShader.LoadFromSource(SCREEN_VERT_SRC, SCREEN_FRAG_SRC);
}

//=============================================================================

void GameApp::destroyScreenQuad() noexcept
{
	if (m_screenQuadVAO != 0)
	{
		glDeleteVertexArrays(1, &m_screenQuadVAO);
		m_screenQuadVAO = 0;
	}
	if (m_screenQuadVBO != 0)
	{
		glDeleteBuffers(1, &m_screenQuadVBO);
		m_screenQuadVBO = 0;
	}
}

//=============================================================================

void GameApp::drawScreenQuad() noexcept
{
	m_screenShader.Bind();
	m_screenShader.SetUniform("uTexture", 0);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_fbo.ColorTexture());

	glBindVertexArray(m_screenQuadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}
