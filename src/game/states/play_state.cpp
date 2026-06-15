#include "stdafx.h"
#include "game/states/play_state.h"
#include "engine/delta_time.h"
#include "engine/input_manager.h"
#include "engine/window.h"
#include "engine/resource_manager.h"
#include "engine/renderer/shader_sources.h"
#include "core/logger.h"
#include "core/exception.h"
#include "engine/renderer/renderer.h"
#include "game/states/settings_state.h"

//=============================================================================

PlayState::PlayState(
	GameStateMachine& machine,
	const Window& window,
	InputManager& input,
	ResourceManager& resources
) noexcept
	: m_machine(machine)
	, m_window(window)
	, m_input(input)
	, m_resources(resources)
{}

PlayState::~PlayState() noexcept = default;

//=============================================================================

void PlayState::OnEnter() noexcept
{
	try
	{
		Logger::Info("PlayState::OnEnter begin");
		m_input.ResetState();
		m_renderer = nullptr;
		m_showDebug = false;
		m_gameMode = GameMode::Exploring;

		m_debugRenderer.Init();
		Logger::Info("PlayState: debug renderer initialized");

		// ---- Shaders ----
		m_dungeonShader = m_resources.GetOrCreateShader("dungeon", DUNGEON_VERT_SRC, DUNGEON_FRAG_SRC);
		if (!m_dungeonShader)
		{
			throw std::runtime_error("Failed to create dungeon shader");
		}
		Logger::Info("PlayState: shader created");

		// ---- Textures from files ----
		m_texFloor = m_resources.LoadTexture("tex_dungeon_floor", "data/texture_08.png", false);
		m_texWall = m_resources.LoadTexture("tex_dungeon_wall", "data/texture_10.png", false);
		m_texCeiling = m_resources.LoadTexture("tex_dungeon_ceiling", "data/texture_11.png", false);

		if (!m_texFloor || !m_texWall || !m_texCeiling)
		{
			throw std::runtime_error("Failed to load dungeon textures");
		}
		Logger::Info("PlayState: textures loaded");

		// ---- Materials ----
		m_matFloor = m_resources.GetOrCreateMaterial("mat_dungeon_floor", *m_dungeonShader, *m_texFloor);
		m_matWall = m_resources.GetOrCreateMaterial("mat_dungeon_wall", *m_dungeonShader, *m_texWall);
		m_matCeiling = m_resources.GetOrCreateMaterial("mat_dungeon_ceiling", *m_dungeonShader, *m_texCeiling);

		if (!m_matFloor || !m_matWall || !m_matCeiling)
		{
			throw std::runtime_error("Failed to create dungeon materials");
		}
		Logger::Info("PlayState: materials created");

		// ---- Dungeon setup ----
		m_dungeon.GenerateTestRoom();
		m_dungeonRenderer.SetFloorMaterial(m_matFloor);
		m_dungeonRenderer.SetWallMaterial(m_matWall);
		m_dungeonRenderer.SetCeilingMaterial(m_matCeiling);
		m_dungeonRenderer.SetNeedsRebuild(true);
		Logger::Info("PlayState: dungeon generated");

		// ---- Config ----
		m_moveRepeatDelay = GetGridMoveRepeatDelayFromConfig();

		// ---- Grid camera ----
		m_camera.SetGridPosition({17, 17, 0}, Direction::North);
		m_camera.SnapToGrid();

		// ---- Input actions for grid movement ----
		m_input.BindAction("GridMoveForward",  SDL_SCANCODE_W);
		m_input.BindAction("GridMoveBackward", SDL_SCANCODE_S);
		m_input.BindAction("TurnLeft",         SDL_SCANCODE_A);
		m_input.BindAction("TurnRight",        SDL_SCANCODE_D);
		m_input.BindAction("StrafeLeft",       SDL_SCANCODE_Q);
		m_input.BindAction("StrafeRight",      SDL_SCANCODE_E);

		// ---- Perspective ----
		m_camera.SetPerspective(60.0f,
			static_cast<float>(m_window.Width()) / static_cast<float>(m_window.Height()),
			0.1f, 100.0f);

		m_initialized = true;
		Logger::Info("PlayState (Dungeon Crawler) initialized");
	}
	catch (const std::exception& e)
	{
		Logger::Error(std::string("PlayState init failed: ") + e.what());
		m_initialized = false;
	}
	catch (...)
	{
		Logger::Error("PlayState init failed with unknown exception");
		m_initialized = false;
	}
}

//=============================================================================

void PlayState::OnExit() noexcept
{
	m_input.ClearActions();
	m_input.SetMouseCaptured(false);
	m_resources.CleanupUnused();
}

void PlayState::OnPause() noexcept
{
	m_input.SetMouseCaptured(false);
}

void PlayState::OnResume() noexcept
{
	SDL_ShowCursor();
}

//=============================================================================

void PlayState::HandleEvent(const SDL_Event& event) noexcept
{
	PROFILE_FUNCTION();

	if (!m_initialized)
	{
		return;
	}

	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_TAB)
	{
		if (m_showDebug)
		{
			exitDebugMode();
		}
		else
		{
			enterDebugMode();
		}
		m_showDebug = !m_showDebug;
	}

	if (event.type == SDL_EVENT_WINDOW_RESIZED)
	{
		const int32_t w = event.window.data1;
		const int32_t h = event.window.data2;
		if (h > 0)
		{
			m_camera.SetAspectRatio(static_cast<float>(w) / static_cast<float>(h));
		}
	}
}

//=============================================================================

void PlayState::enterDebugMode() noexcept
{
	// Save current grid camera state before switching to free camera
	m_savedCamera.position = m_camera.Position();
	m_savedCamera.yaw = m_camera.Yaw();
	m_savedCamera.pitch = m_camera.Pitch();
	m_savedCamera.gridPos = m_camera.GetGridPosition();
	m_savedCamera.facing = m_camera.Facing();
	m_savedCamera.isAnimating = m_camera.IsAnimating();

	m_input.SetMouseCaptured(true);
}

void PlayState::exitDebugMode() noexcept
{
	// Restore grid camera state
	m_camera.SetPosition(m_savedCamera.position);
	m_camera.SetRotation(m_savedCamera.yaw, m_savedCamera.pitch);
	m_camera.SetGridPosition(m_savedCamera.gridPos, m_savedCamera.facing);

	m_input.SetMouseCaptured(false);
}

//=============================================================================

void PlayState::FixedUpdate(float fixedDt) noexcept
{
	PROFILE_FUNCTION();
	(void)fixedDt;
	if (!m_initialized)
	{
		return;
	}
}

//=============================================================================

void PlayState::Update(const DeltaTime& dt) noexcept
{
	PROFILE_FUNCTION();
	if (!m_initialized)
	{
		return;
	}

	if (m_showDebug)
	{
		// ---- Debug / Free camera ----
		const float speed = 3.0f * dt.Seconds();
		if (m_input.IsKeyDown(SDL_SCANCODE_W)) m_camera.MoveForward(speed);
		if (m_input.IsKeyDown(SDL_SCANCODE_S)) m_camera.MoveForward(-speed);
		if (m_input.IsKeyDown(SDL_SCANCODE_A)) m_camera.MoveRight(-speed);
		if (m_input.IsKeyDown(SDL_SCANCODE_D)) m_camera.MoveRight(speed);
		if (m_input.IsKeyDown(SDL_SCANCODE_SPACE)) m_camera.MoveUp(speed);
		if (m_input.IsKeyDown(SDL_SCANCODE_LSHIFT)) m_camera.MoveUp(-speed);

		if (m_input.IsMouseCaptured())
		{
			constexpr float mouseSensitivity = 0.15f;
			const float yawDelta = m_input.MouseDeltaX() * mouseSensitivity;
			const float pitchDelta = -m_input.MouseDeltaY() * mouseSensitivity;
			m_camera.Rotate(yawDelta, pitchDelta);
		}
	}
	else
	{
		// Turn system — process states in order
		processTurnWaiting();
		processEdgeActions();
		processHeldRepeat(dt);

		// If animation finished while in TurnAnimating, go back to Exploring
		if (m_gameMode == GameMode::TurnAnimating && !m_camera.IsAnimating())
		{
			m_gameMode = GameMode::Exploring;
		}
	}

	// ---- Camera animation ----
	m_camera.UpdateAnimation(static_cast<float>(dt.Seconds()));

	m_audio.Update();
}

//=============================================================================

bool PlayState::isMovementAction(std::string_view name) const noexcept
{
	return (name != "TurnLeft" && name != "TurnRight");
}

//=============================================================================

bool PlayState::isWalkableAction(std::string_view name) const noexcept
{
	auto getDelta = [&]() -> glm::ivec2
	{
		if (name == "GridMoveForward")  return  DirectionToVec(m_camera.Facing());
		if (name == "GridMoveBackward") return -DirectionToVec(m_camera.Facing());
		if (name == "StrafeLeft")       return  DirectionToVec(NextDirection(m_camera.Facing(), false));
		/* StrafeRight */               return  DirectionToVec(NextDirection(m_camera.Facing(), true));
	};

	// Rotation actions are always walkable
	if (!isMovementAction(name))
	{
		return true;
	}

	glm::ivec2 delta = getDelta();
	GridPosition target(
		m_camera.GetGridPosition().row + delta.x,
		m_camera.GetGridPosition().col + delta.y,
		m_camera.GetGridPosition().floor
	);
	return m_dungeon.IsWalkable(target);
}

//=============================================================================

void PlayState::doGridAction(std::string_view name) noexcept
{
	if (name == "GridMoveForward")  m_camera.MoveForward();
	else if (name == "GridMoveBackward") m_camera.MoveBackward();
	else if (name == "TurnLeft")    m_camera.TurnLeft();
	else if (name == "TurnRight")   m_camera.TurnRight();
	else if (name == "StrafeLeft")  m_camera.MoveLeft();
	else                            m_camera.MoveRight();
}

//=============================================================================

void PlayState::processEdgeActions() noexcept
{
	// Edge-triggered actions are processed during Exploring and TurnAnimating.
	// TurnWaiting processes instantly (same frame), so this runs in Exploring
	// the next frame. TurnWaiting itself blocks new input.
	if (m_gameMode != GameMode::Exploring && m_gameMode != GameMode::TurnAnimating)
	{
		return;
	}

	static constexpr std::string_view ACTION_NAMES[] =
	{
		"GridMoveForward",
		"GridMoveBackward",
		"TurnLeft",
		"TurnRight",
		"StrafeLeft",
		"StrafeRight"
	};

	for (std::string_view name : ACTION_NAMES)
	{
		if (!m_input.IsActionPressed(name))
		{
			continue;
		}

		// If camera is animating, cancel current animation
		if (m_camera.IsAnimating())
		{
			m_camera.SnapToGrid();
		}

		bool movement = isMovementAction(name);

		// Movement actions: check walkability
		if (movement && !isWalkableAction(name))
		{
			m_moveRepeatTimer = 0.0f;
			break;
		}

		doGridAction(name);
		m_moveRepeatTimer = 0.0f;

		// Movement actions consume a turn
		if (movement)
		{
			m_gameMode = GameMode::TurnWaiting;
		}

		break;
	}
}

//=============================================================================

void PlayState::processHeldRepeat(const DeltaTime& dt) noexcept
{
	// Auto-repeat only during Exploring, when camera is idle
	if (m_gameMode != GameMode::Exploring)
	{
		return;
	}
	if (m_camera.IsAnimating())
	{
		return;
	}

	static constexpr std::string_view ACTION_NAMES[] =
	{
		"GridMoveForward",
		"GridMoveBackward",
		"TurnLeft",
		"TurnRight",
		"StrafeLeft",
		"StrafeRight"
	};

	// If any key is freshly pressed (edge), skip repeat this frame
	for (std::string_view name : ACTION_NAMES)
	{
		if (m_input.IsActionPressed(name))
		{
			return;
		}
	}

	// Find a held key
	std::string_view heldAction;
	for (std::string_view name : ACTION_NAMES)
	{
		if (m_input.IsActionDown(name))
		{
			heldAction = name;
			break;
		}
	}

	if (heldAction.empty())
	{
		m_moveRepeatTimer = 0.0f;
		return;
	}

	m_moveRepeatTimer += static_cast<float>(dt.Seconds());
	if (m_moveRepeatTimer >= m_moveRepeatDelay)
	{
		bool movement = isMovementAction(heldAction);

		// Check walkability for movement actions
		if (!movement || isWalkableAction(heldAction))
		{
			doGridAction(heldAction);

			if (movement)
			{
				m_gameMode = GameMode::TurnWaiting;
			}
		}
		m_moveRepeatTimer = 0.0f;
	}
}

//=============================================================================

void PlayState::processTurnWaiting() noexcept
{
	if (m_gameMode != GameMode::TurnWaiting)
	{
		return;
	}

	// Advance turn queue (no-op with only player)
	m_turnQueue.Advance();

	// Future: process enemy actions here

	// Transition to Exploring or TurnAnimating depending on camera state
	if (m_camera.IsAnimating())
	{
		m_gameMode = GameMode::TurnAnimating;
	}
	else
	{
		m_gameMode = GameMode::Exploring;
	}
}

//=============================================================================

void PlayState::RenderScene(Renderer& renderer) noexcept
{
	PROFILE_FUNCTION();
	if (!m_initialized)
	{
		return;
	}

	m_renderer = &renderer;

	// Rebuild dungeon geometry if changed
	m_dungeonRenderer.Build(m_dungeon);

	renderer.BeginFrame(m_camera.ViewMatrix(), m_camera.ProjectionMatrix());

	m_dungeonRenderer.Submit(renderer);

	m_debugRenderer.SetEnabled(m_showDebug);

	renderer.EndFrame();
}

//=============================================================================

void PlayState::RenderOverlay(const glm::mat4& viewProj) noexcept
{
	PROFILE_FUNCTION();

	if (m_showDebug && m_initialized)
	{
		m_debugRenderer.SetViewProj(viewProj);
		m_debugRenderer.Flush(m_camera.ViewMatrix(), m_camera.ProjectionMatrix());
	}
}

//=============================================================================

void PlayState::Render() noexcept
{
	if (!m_initialized)
	{
		ImGui::Begin("Error");
		ImGui::Text("Failed to initialize PlayState. Check logs.");
		ImGui::End();
		return;
	}

	ImGui::Begin("Dungeon Crawler");
	const int32_t chunkSize = m_dungeon.ChunkSize();
	ImGui::Text("Chunk: %dx%d", chunkSize, chunkSize);
	ImGui::Separator();
	ImGui::Text("Controls (grid mode):");
	ImGui::Text("  W/S - Move Forward/Backward");
	ImGui::Text("  A/D - Turn Left/Right");
	ImGui::Text("  Q/E - Strafe Left/Right");
	ImGui::Separator();
	ImGui::Text("Tab: Toggle debug mode [%s]", m_showDebug ? "ON" : "OFF");

	if (ImGui::Button("Back to Menu"))
	{
		m_machine.ReplaceState("MainMenu");
	}
	ImGui::End();

	// Game mode
	ImGui::Begin("Turn System");
	auto modeName = [](GameMode m) -> const char*
	{
		switch (m)
		{
			case GameMode::Exploring:     return "Exploring";
			case GameMode::TurnWaiting:   return "TurnWaiting";
			case GameMode::TurnAnimating: return "TurnAnimating";
			case GameMode::CombatTurn:    return "CombatTurn";
			case GameMode::GameOver:      return "GameOver";
		}
		return "Unknown";
	};
	ImGui::Text("Game Mode: %s", modeName(m_gameMode));
	ImGui::Text("Turn Queue Actor: %d", m_turnQueue.CurrentActor());
	ImGui::Text("Is Player Turn:  %s", m_turnQueue.IsPlayerTurn() ? "yes" : "no");
	ImGui::Text("Move Repeat Delay: %.3f s", m_moveRepeatDelay);
	ImGui::End();

	// Camera debug
	ImGui::Begin("Camera Debug");
	const glm::vec3 pos = m_camera.Position();
	const glm::vec3 fwd = m_camera.Forward();
	ImGui::Text("Position:  %.2f, %.2f, %.2f", pos.x, pos.y, pos.z);
	ImGui::Text("Forward:   %.2f, %.2f, %.2f", fwd.x, fwd.y, fwd.z);
	ImGui::Text("Grid pos:  [%d, %d, %d]",
		m_camera.GetGridPosition().row,
		m_camera.GetGridPosition().col,
		m_camera.GetGridPosition().floor);
	ImGui::Text("Facing:    %s",
		m_camera.Facing() == Direction::North ? "North" :
		m_camera.Facing() == Direction::East  ? "East"  :
		m_camera.Facing() == Direction::South ? "South" : "West");
	ImGui::Text("Animating: %s", m_camera.IsAnimating() ? "yes" : "no");
	ImGui::Separator();
	if (m_renderer != nullptr)
	{
		ImGui::Text("Meshes drawn: %d / %d", m_renderer->DrawnMeshes(), m_renderer->TotalMeshes());
	}
	else
	{
		ImGui::Text("Meshes drawn: N/A");
	}
	ImGui::End();

	// Grid position info
	ImGui::Begin("Dungeon Info");
	const GridPosition gp = m_camera.GetGridPosition();
	const Cell& cell = m_dungeon.GetCell(gp);
	ImGui::Text("Wall:  %s", cell.isWall ? "yes" : "no");
	ImGui::Text("Floor: %s", cell.hasFloor ? "yes" : "no");
	ImGui::Text("Walkable: %s", cell.IsWalkable() ? "yes" : "no");
	ImGui::End();
}
