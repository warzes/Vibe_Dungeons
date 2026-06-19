#include "stdafx.h"
#include "game/states/overworld_state.h"
#include "engine/delta_time.h"
#include "engine/input_manager.h"
#include "engine/window.h"
#include "engine/resource_manager.h"
#include "engine/renderer/shader_sources.h"
#include "engine/renderer/renderer.h"
#include "core/logger.h"
#include "core/exception.h"
#include "core/json_data_manager.h"
#include "core/file_io.h"
#include "game/states/settings_state.h"

//=============================================================================

OverworldState::OverworldState(
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

OverworldState::~OverworldState() noexcept = default;

//=============================================================================

Texture* OverworldState::createTerrainPalette() noexcept
{
	// Palette texture: each terrain type is one pixel column.
	// Shader samples with NEAREST, texCoord.x maps to pixel column.
	static constexpr int32_t W = 8;
	static constexpr int32_t H = 1;

	// RGBA byte array (OpenGL GL_RGBA expects R,G,B,A in order)
	uint8_t pixels[W * 4] =
	{
		76, 175, 80, 255,    // 0: Grassland
		46, 125, 50, 255,    // 1: Forest
		121, 85, 72, 255,    // 2: Mountain
		21, 101, 192, 255,   // 3: Water
		141, 110, 99, 255,   // 4: Road
		255, 204, 128, 255,  // 5: Desert
		255, 215, 0, 255,    // 6: Location (gold)
		136, 136, 136, 255,  // 7: Unused
	};

	auto* tex = new Texture();
	tex->CreateFromRaw(W, H, reinterpret_cast<const uint32_t*>(pixels));
	return tex;
}

//=============================================================================

void OverworldState::OnEnter() noexcept
{
	try
	{
		Logger::Info("OverworldState::OnEnter begin");
		m_input.ResetState();
		m_renderer = nullptr;

		// ---- Shader ----
		m_overworldShader = m_resources.GetOrCreateShader("overworld",
			OVERWORLD_VERT_SRC, OVERWORLD_FRAG_SRC);
		if (!m_overworldShader)
		{
			throw std::runtime_error("Failed to create overworld shader");
		}
		Logger::Info("Overworld: shader created");

		// ---- Terrain palette texture (procedural) ----
		m_terrainPalette = createTerrainPalette();
		if (!m_terrainPalette)
		{
			throw std::runtime_error("Failed to create terrain palette");
		}
		Logger::Info("Overworld: palette texture created");

		// ---- Materials ----
		m_floorMaterial = m_resources.GetOrCreateMaterial(
			"mat_overworld_floor", *m_overworldShader, *m_terrainPalette);
		m_wallMaterial = m_resources.GetOrCreateMaterial(
			"mat_overworld_wall", *m_overworldShader, *m_terrainPalette);
		m_locationMaterial = m_resources.GetOrCreateMaterial(
			"mat_overworld_loc", *m_overworldShader, *m_terrainPalette);

		if (!m_floorMaterial || !m_wallMaterial || !m_locationMaterial)
		{
			throw std::runtime_error("Failed to create overworld materials");
		}
		Logger::Info("Overworld: materials created");

		// ---- Overworld setup ----
		std::string buf = FileReadString("data/locations.json");
		if (!buf.empty())
		{
			json parsed = json::parse(buf, nullptr, false);
			if (!parsed.is_discarded())
			{
				m_overworld.LoadLocations(parsed);
				Logger::Info("Overworld: locations loaded from file");
			}
		}

		m_overworld.GenerateDefaultMap();
		Logger::Info("Overworld: map generated");

		m_overworldRenderer.SetFloorMaterial(m_floorMaterial);
		m_overworldRenderer.SetWallMaterial(m_wallMaterial);
		m_overworldRenderer.SetLocationMaterial(m_locationMaterial);
		m_overworldRenderer.SetNeedsRebuild(true);

		// ---- Start position: SW corner ----
		m_camera.SetGridPosition({58, 5, 0}, Direction::North);
		m_camera.SnapToGrid();
		m_overworld.MarkVisited({58, 5, 0});

		// ---- Config ----
		m_moveRepeatDelay = GetGridMoveRepeatDelayFromConfig();

		// ---- Perspective ----
		m_camera.SetPerspective(60.0f,
			static_cast<float>(m_window.Width()) / static_cast<float>(m_window.Height()),
			0.1f, 200.0f);

		// ---- Input actions ----
		m_input.BindAction("GridMoveForward",  SDL_SCANCODE_W);
		m_input.BindAction("GridMoveBackward", SDL_SCANCODE_S);
		m_input.BindAction("TurnLeft",         SDL_SCANCODE_A);
		m_input.BindAction("TurnRight",        SDL_SCANCODE_D);
		m_input.BindAction("StrafeLeft",       SDL_SCANCODE_Q);
		m_input.BindAction("StrafeRight",      SDL_SCANCODE_E);
		m_input.BindAction("Action_Interact",  SDL_SCANCODE_SPACE);

		m_initialized = true;
		Logger::Info("OverworldState initialized");
	}
	catch (const std::exception& e)
	{
		Logger::Error(std::string("OverworldState init failed: ") + e.what());
		m_initialized = false;
	}
	catch (...)
	{
		Logger::Error("OverworldState init failed with unknown exception");
		m_initialized = false;
	}
}

//=============================================================================

void OverworldState::OnExit() noexcept
{
	m_input.ClearActions();
	m_input.SetMouseCaptured(false);

	// Clean up palette texture (created manually, not in ResourceManager)
	if (m_terrainPalette)
	{
		delete m_terrainPalette;
		m_terrainPalette = nullptr;
	}
	m_resources.CleanupUnused();
}

void OverworldState::OnPause() noexcept
{
	m_input.SetMouseCaptured(false);
}

void OverworldState::OnResume() noexcept
{
	SDL_ShowCursor();
}

//=============================================================================

void OverworldState::HandleEvent(const SDL_Event& event) noexcept
{
	if (!m_initialized) return;

	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_M)
	{
		m_showMap = !m_showMap;
	}

	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F1)
	{
		m_showDebug = !m_showDebug;
	}

	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
	{
		m_machine.ReplaceState("MainMenu");
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

void OverworldState::FixedUpdate(float fixedDt) noexcept
{
	(void)fixedDt;
}

//=============================================================================

void OverworldState::Update(const DeltaTime& dt) noexcept
{
	if (!m_initialized) return;

	processEdgeActions();
	processInteraction();
	processHeldRepeat(dt);

	m_camera.UpdateAnimation(static_cast<float>(dt.Seconds()));
}

//=============================================================================

bool OverworldState::isWalkableAction(std::string_view name) const noexcept
{
	if (name == "TurnLeft" || name == "TurnRight")
	{
		return true;
	}

	auto getDelta = [&]() -> glm::ivec2
	{
		if (name == "GridMoveForward")  return  DirectionToVec(m_camera.Facing());
		if (name == "GridMoveBackward") return -DirectionToVec(m_camera.Facing());
		if (name == "StrafeLeft")       return  DirectionToVec(NextDirection(m_camera.Facing(), false));
		return DirectionToVec(NextDirection(m_camera.Facing(), true));
	};

	glm::ivec2 delta = getDelta();
	GridPosition target(
		m_camera.GetGridPosition().row + delta.x,
		m_camera.GetGridPosition().col + delta.y,
		0
	);
	return m_overworld.IsWalkable(target);
}

//=============================================================================

void OverworldState::doGridAction(std::string_view name) noexcept
{
	if (name == "GridMoveForward")  m_camera.MoveForward();
	else if (name == "GridMoveBackward") m_camera.MoveBackward();
	else if (name == "TurnLeft")    m_camera.TurnLeft();
	else if (name == "TurnRight")   m_camera.TurnRight();
	else if (name == "StrafeLeft")  m_camera.MoveLeft();
	else                            m_camera.MoveRight();

	// Mark new position as visited
	GridPosition newPos = m_camera.GetGridPosition();
	m_overworld.MarkVisited(newPos);
}

//=============================================================================

void OverworldState::processEdgeActions() noexcept
{
	for (std::string_view name : GRID_ACTION_NAMES)
	{
		if (!m_input.IsActionPressed(name))
		{
			continue;
		}

		if (m_camera.IsAnimating())
		{
			m_camera.SnapToGrid();
		}

		bool movement = (name != "TurnLeft" && name != "TurnRight");

		if (movement && !isWalkableAction(name))
		{
			m_moveRepeatTimer = 0.0f;
			break;
		}

		doGridAction(name);
		m_moveRepeatTimer = 0.0f;
		break;
	}
}

//=============================================================================

void OverworldState::processHeldRepeat(const DeltaTime& dt) noexcept
{
	if (m_camera.IsAnimating())
	{
		return;
	}

	for (std::string_view name : GRID_ACTION_NAMES)
	{
		if (m_input.IsActionPressed(name))
		{
			return;
		}
	}

	std::string_view heldAction;
	for (std::string_view name : GRID_ACTION_NAMES)
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
		bool movement = (heldAction != "TurnLeft" && heldAction != "TurnRight");
		if (!movement || isWalkableAction(heldAction))
		{
			doGridAction(heldAction);
		}
		m_moveRepeatTimer = 0.0f;
	}
}

//=============================================================================

void OverworldState::processInteraction() noexcept
{
	if (!m_input.IsActionPressed("Action_Interact"))
	{
		return;
	}

	GridPosition pos = m_camera.GetGridPosition();
	const OverworldLocation* loc = m_overworld.FindLocationAt(pos);
	if (!loc)
	{
		// Also check the cell in front
		glm::ivec2 fwd = DirectionToVec(m_camera.Facing());
		GridPosition front = {pos.row + fwd.x, pos.col + fwd.y, 0};
		loc = m_overworld.FindLocationAt(front);
	}

	if (loc)
	{
		Logger::Info(std::string("Interacting with location: ") + loc->name);
		// Future: transition to specialized state
	}
}

//=============================================================================

void OverworldState::RenderScene(Renderer& renderer) noexcept
{
	if (!m_initialized) return;

	m_renderer = &renderer;

	m_overworldRenderer.Build(m_overworld);

	renderer.BeginFrame(m_camera.ViewMatrix(), m_camera.ProjectionMatrix());

	m_overworldRenderer.Submit(renderer);

	renderer.EndFrame();
}

//=============================================================================

void OverworldState::RenderOverlay(const glm::mat4& viewProj) noexcept
{
	(void)viewProj;
}

//=============================================================================

void OverworldState::Render() noexcept
{
	if (!m_initialized)
	{
		ImGui::Begin("Error");
		ImGui::Text("Failed to initialize OverworldState. Check logs.");
		ImGui::End();
		return;
	}

	// ---- Controls panel ----
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(260, 130), ImGuiCond_FirstUseEver);
	ImGui::Begin("Overworld");
	ImGui::Text("World: %dx%d", OVERWORLD_SIZE, OVERWORLD_SIZE);
	ImGui::Separator();
	ImGui::Text("W/S \tMove Fwd/Back");
	ImGui::Text("A/D \tTurn L/R");
	ImGui::Text("Q/E \tStrafe L/R");
	ImGui::Text("Space\tInteract");
	ImGui::Separator();
	ImGui::Text("M: Map  F1: Debug  Esc: Menu");

	// Show current terrain
	GridPosition gp = m_camera.GetGridPosition();
	const OverworldCell& cell = m_overworld.GetCell(gp);
	static const char* TERRAIN_NAMES[] =
	{
		"Grassland", "Forest", "Mountain", "Water", "Road", "Desert"
	};
	const char* terrainName = (static_cast<int32_t>(cell.terrain) < 6)
		? TERRAIN_NAMES[static_cast<int32_t>(cell.terrain)]
		: "Unknown";
	ImGui::Text("Terrain: %s", terrainName);

	const OverworldLocation* loc = m_overworld.FindLocationAt(gp);
	if (loc)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
			"Location: %s", loc->name.c_str());
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
			"%s", loc->description.c_str());
		ImGui::Text("Press Space to enter");
	}
	else
	{
		glm::ivec2 fwd = DirectionToVec(m_camera.Facing());
		GridPosition front = {gp.row + fwd.x, gp.col + fwd.y, 0};
		const OverworldLocation* frontLoc = m_overworld.FindLocationAt(front);
		if (frontLoc)
		{
			ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
				"Ahead: %s", frontLoc->name.c_str());
			ImGui::Text("Press Space to enter");
		}
	}

	if (ImGui::Button("Back to Menu"))
	{
		m_machine.ReplaceState("MainMenu");
	}
	ImGui::End();

	// ---- Position info ----
	ImGui::SetNextWindowPos(ImVec2(0, 140), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(260, 50), ImGuiCond_FirstUseEver);
	ImGui::Begin("Position");
	ImGui::Text("Grid: [%d, %d]", gp.row, gp.col);
	ImGui::Text("Facing: %s",
		m_camera.Facing() == Direction::North ? "North" :
		m_camera.Facing() == Direction::East  ? "East"  :
		m_camera.Facing() == Direction::South ? "South" : "West");
	ImGui::End();

	// ---- Minimap ----
	renderMinimap();

	// ---- Full map (M key) ----
	if (m_showMap)
	{
		renderFullMap();
	}

	// ---- Debug ----
	if (m_showDebug)
	{
		ImGui::SetNextWindowPos(ImVec2(0, 200), ImGuiCond_FirstUseEver, ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(260, 120), ImGuiCond_FirstUseEver);
		ImGui::Begin("Overworld Debug");
		const glm::vec3 camPos = m_camera.Position();
		ImGui::Text("Cam Pos: %.2f, %.2f, %.2f", camPos.x, camPos.y, camPos.z);
		ImGui::Text("Animating: %s", m_camera.IsAnimating() ? "yes" : "no");
		if (m_renderer)
		{
			ImGui::Text("Meshes: %d / %d", m_renderer->DrawnMeshes(), m_renderer->TotalMeshes());
		}
		ImGui::Text("Locations: %zu", m_overworld.Locations().size());
		ImGui::End();
	}
}

//=============================================================================

void OverworldState::renderMinimap() noexcept
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 ws = viewport->WorkSize;

	ImGui::SetNextWindowPos(ImVec2(ws.x - 210, 0), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(200, 200), ImGuiCond_FirstUseEver);
	ImGui::Begin("Minimap", nullptr,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	const int32_t radius = 4;
	GridPosition center = m_camera.GetGridPosition();

	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const float cellSize = (std::min)(avail.x, avail.y) / static_cast<float>(radius * 2 + 1);
	const float mmSize = cellSize * static_cast<float>(radius * 2 + 1);

	const float offsetX = (avail.x - mmSize) * 0.5f;
	const float offsetY = (avail.y - mmSize) * 0.5f;

	ImDrawList* draw = ImGui::GetWindowDrawList();
	const ImVec2 origin = ImGui::GetCursorScreenPos() + ImVec2(offsetX, offsetY);

	for (int32_t dr = -radius; dr <= radius; ++dr)
	{
		for (int32_t dc = -radius; dc <= radius; ++dc)
		{
			int32_t r = center.row + dr;
			int32_t c = center.col + dc;
			ImVec2 p0 = origin + ImVec2(
				static_cast<float>(dc + radius) * cellSize,
				static_cast<float>(dr + radius) * cellSize);
			ImVec2 p1 = p0 + ImVec2(cellSize, cellSize);

			uint32_t color;
			if (!Overworld::InBounds(r, c))
			{
				color = IM_COL32(0, 0, 0, 255);
			}
			else if (!m_overworld.IsVisited({r, c, 0}))
			{
				color = IM_COL32(10, 10, 10, 255);
			}
			else
			{
				const OverworldCell& cell = m_overworld.GetCell(r, c);
				switch (cell.terrain)
				{
					case TerrainType::Grassland: color = IM_COL32(76, 175, 80, 255); break;
					case TerrainType::Forest:    color = IM_COL32(46, 125, 50, 255); break;
					case TerrainType::Mountain:  color = IM_COL32(121, 85, 72, 255); break;
					case TerrainType::Water:     color = IM_COL32(21, 101, 192, 255); break;
					case TerrainType::Road:      color = IM_COL32(141, 110, 99, 255); break;
					case TerrainType::Desert:    color = IM_COL32(255, 204, 128, 255); break;
					default:                     color = IM_COL32(80, 80, 80, 255); break;
				}
				if (cell.hasLocation)
				{
					color = IM_COL32(255, 215, 0, 255);
				}
			}

			draw->AddRectFilled(p0, p1, color);
		}
	}

	// Player dot
	ImVec2 centerPos = origin + ImVec2(
		static_cast<float>(radius) * cellSize,
		static_cast<float>(radius) * cellSize);
	draw->AddCircleFilled(centerPos, cellSize * 0.3f, IM_COL32(40, 220, 40, 240), 10);
	glm::ivec2 fwd = DirectionToVec(m_camera.Facing());
	ImVec2 tip = centerPos + ImVec2(
		static_cast<float>(fwd.y) * cellSize * 0.4f,
		static_cast<float>(fwd.x) * cellSize * 0.4f);
	draw->AddLine(centerPos, tip, IM_COL32(100, 255, 100, 240), 2.0f);

	ImGui::Dummy(ImVec2(mmSize, mmSize));
	ImGui::End();
}

//=============================================================================

void OverworldState::renderFullMap() noexcept
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 ws = viewport->WorkSize;

	ImGui::SetNextWindowPos(ImVec2(ws.x * 0.5f - 275.0f, 50.0f), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(550, 550), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("World Map", &m_showMap,
		ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		ImGui::End();
		return;
	}

	const int32_t mapSize = OVERWORLD_SIZE;
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const float cellSize = (std::min)(avail.x, avail.y) / static_cast<float>(mapSize);
	const float totalSize = cellSize * static_cast<float>(mapSize);

	const float offsetX = (avail.x - totalSize) * 0.5f;
	const float offsetY = (avail.y - totalSize) * 0.5f;

	ImDrawList* draw = ImGui::GetWindowDrawList();
	const ImVec2 origin = ImGui::GetCursorScreenPos() + ImVec2(offsetX, offsetY);

	for (int32_t r = 0; r < mapSize; ++r)
	{
		for (int32_t c = 0; c < mapSize; ++c)
		{
			ImVec2 p0 = origin + ImVec2(static_cast<float>(c) * cellSize,
				static_cast<float>(r) * cellSize);
			ImVec2 p1 = p0 + ImVec2(cellSize, cellSize);

			uint32_t color;
			if (!m_overworld.IsVisited({r, c, 0}))
			{
				color = IM_COL32(10, 10, 15, 255);
			}
			else
			{
				const OverworldCell& cell = m_overworld.GetCell(r, c);
				switch (cell.terrain)
				{
					case TerrainType::Grassland: color = IM_COL32(76, 175, 80, 255); break;
					case TerrainType::Forest:    color = IM_COL32(46, 125, 50, 255); break;
					case TerrainType::Mountain:  color = IM_COL32(121, 85, 72, 255); break;
					case TerrainType::Water:     color = IM_COL32(21, 101, 192, 255); break;
					case TerrainType::Road:      color = IM_COL32(141, 110, 99, 255); break;
					case TerrainType::Desert:    color = IM_COL32(255, 204, 128, 255); break;
					default:                     color = IM_COL32(80, 80, 80, 255); break;
				}
			}

			draw->AddRectFilled(p0, p1, color);
		}
	}

	// Location markers
	for (const auto& loc : m_overworld.Locations())
	{
		ImVec2 centerPos = origin + ImVec2(
			(static_cast<float>(loc.position.col) + 0.5f) * cellSize,
			(static_cast<float>(loc.position.row) + 0.5f) * cellSize);
		draw->AddCircleFilled(centerPos, cellSize * 0.5f,
			IM_COL32(255, 215, 0, 220), 12);
		draw->AddCircle(centerPos, cellSize * 0.55f,
			IM_COL32(255, 255, 200, 200), 12, 1.5f);

		ImVec2 labelPos = centerPos + ImVec2(cellSize * 0.6f, -cellSize * 0.3f);
		draw->AddText(labelPos, IM_COL32(255, 255, 200, 240), loc.name.c_str());
	}

	// Player dot
	GridPosition ppos = m_camera.GetGridPosition();
	ImVec2 playerPos = origin + ImVec2(
		(static_cast<float>(ppos.col) + 0.5f) * cellSize,
		(static_cast<float>(ppos.row) + 0.5f) * cellSize);
	draw->AddCircleFilled(playerPos, cellSize * 0.4f,
		IM_COL32(40, 220, 40, 240), 12);
	glm::ivec2 fwd = DirectionToVec(m_camera.Facing());
	ImVec2 tip = playerPos + ImVec2(
		static_cast<float>(fwd.y) * cellSize * 0.5f,
		static_cast<float>(fwd.x) * cellSize * 0.5f);
	draw->AddLine(playerPos, tip, IM_COL32(100, 255, 100, 240), 2.5f);

	// Grid lines every 8 cells
	for (int32_t i = 0; i <= mapSize; i += 8)
	{
		float pos = static_cast<float>(i) * cellSize;
		draw->AddLine(origin + ImVec2(0.0f, pos),
			origin + ImVec2(totalSize, pos),
			IM_COL32(50, 50, 50, 60), 0.5f);
		draw->AddLine(origin + ImVec2(pos, 0.0f),
			origin + ImVec2(pos, totalSize),
			IM_COL32(50, 50, 50, 60), 0.5f);
	}

	// Legend
	ImVec2 legendPos = origin + ImVec2(totalSize + 10.0f, 0.0f);
	struct LegendEntry { const char* name; uint32_t color; };
	static const LegendEntry LEGEND[] = {
		{"You",         IM_COL32(40, 220, 40, 255)},
		{"Location",    IM_COL32(255, 215, 0, 220)},
		{"Grassland",   IM_COL32(76, 175, 80, 255)},
		{"Forest",      IM_COL32(46, 125, 50, 255)},
		{"Mountain",    IM_COL32(121, 85, 72, 255)},
		{"Water",       IM_COL32(21, 101, 192, 255)},
		{"Road",        IM_COL32(141, 110, 99, 255)},
		{"Desert",      IM_COL32(255, 204, 128, 255)},
		{"Unexplored",  IM_COL32(10, 10, 15, 255)},
	};
	for (const auto& entry : LEGEND)
	{
		draw->AddRectFilled(legendPos, legendPos + ImVec2(8, 8), entry.color);
		draw->AddText(legendPos + ImVec2(12, -1),
			IM_COL32(180, 180, 180, 255), entry.name);
		legendPos.y += 14.0f;
	}

	ImGui::Dummy(ImVec2(totalSize, totalSize));
	ImGui::End();
}
