#include "stdafx.h"
#include "game/states/overworld_state.h"
#include "game/states/combat_encounter_state.h"
#include "engine/delta_time.h"
#include "engine/input_manager.h"
#include "engine/window.h"
#include "engine/resource_manager.h"
#include "engine/renderer/shader_sources.h"
#include "engine/renderer/renderer.h"
#include "core/logger.h"
#include "core/exception.h"
#include "core/file_io.h"
#include "game/states/settings_state.h"

//=============================================================================

static const char* TERRAIN_NAMES[] =
{
	"Grassland", "Forest", "Mountain", "Water", "Road", "Desert"
};

static const char* TERRAIN_KEY[] =
{
	"Grassland", "Forest", "Mountain", "Water", "Road", "Desert", nullptr
};

static const char* terrainKey(TerrainType t) noexcept
{
	int32_t idx = static_cast<int32_t>(t);
	return (idx >= 0 && idx < 6) ? TERRAIN_KEY[idx] : nullptr;
}

//=============================================================================

OverworldState::OverworldState(
	GameStateMachine& machine,
	const Window& window,
	InputManager& input,
	ResourceManager& resources,
	Character* pendingCharacter
) noexcept
	: m_machine(machine)
	, m_window(window)
	, m_input(input)
	, m_resources(resources)
	, m_pendingCharacter(pendingCharacter)
	, m_rng(std::random_device{}())
{}

OverworldState::~OverworldState() noexcept = default;

//=============================================================================

Texture* OverworldState::createTerrainPalette() noexcept
{
	static constexpr int32_t W = 8;
	static constexpr int32_t H = 1;

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

void OverworldState::loadEncounters() noexcept
{
	m_encounterTable.clear();

	std::string buf = FileReadString("data/overworld_encounters.json");
	if (buf.empty())
	{
		Logger::Warn("overworld_encounters.json not found, encounters disabled");
		return;
	}

	json root = json::parse(buf, nullptr, false);
	if (root.is_discarded() || !root.contains("terrains"))
	{
		Logger::Warn("overworld_encounters.json: invalid format");
		return;
	}

	for (const auto& terrainEntry : root["terrains"])
	{
		std::string name = terrainEntry.value("name", "");
		if (name.empty()) continue;

		std::vector<OverworldEncounterDef> defs;
		for (const auto& enc : terrainEntry["encounters"])
		{
			OverworldEncounterDef def;
			def.weight = enc.value("weight", 1);
			for (const auto& mon : enc["monsters"])
			{
				OverworldEncounterEntry entry;
				entry.monsterTypeId = mon.value("type", "slime");
				entry.count = mon.value("count", 1);
				entry.level = mon.value("level", 1);
				def.entries.push_back(std::move(entry));
			}
			defs.push_back(std::move(def));
		}
		m_encounterTable[name] = std::move(defs);
	}

	Logger::Info("Overworld: encounters loaded");
}

//=============================================================================

void OverworldState::triggerRandomEncounter() noexcept
{
	GridPosition pos = m_camera.GetGridPosition();
	const OverworldCell& cell = m_overworld.GetCell(pos);

	float chance = cell.EncounterChance();
	if (chance <= 0.0f) return;

	// Roll the dice
	std::uniform_real_distribution<float> dist(0.0f, 1.0f);
	if (dist(m_rng) >= chance) return;

	const char* key = terrainKey(cell.terrain);
	if (!key) return;

	auto it = m_encounterTable.find(key);
	if (it == m_encounterTable.end() || it->second.empty()) return;

	const auto& defs = it->second;

	// Weighted random selection
	int32_t totalWeight = 0;
	for (const auto& d : defs) totalWeight += d.weight;
	if (totalWeight <= 0) return;

	std::uniform_int_distribution<int32_t> pick(0, totalWeight - 1);
	int32_t roll = pick(m_rng);

	const OverworldEncounterDef* chosen = nullptr;
	for (const auto& d : defs)
	{
		if (roll < d.weight)
		{
			chosen = &d;
			break;
		}
		roll -= d.weight;
	}
	if (!chosen || chosen->entries.empty()) return;

	Logger::Info("Overworld: random encounter triggered at (" +
		std::to_string(pos.row) + ", " + std::to_string(pos.col) + ")");

	// Store spawn entries for CombatEncounterState
	std::vector<EncounterSpawnEntry> spawnList;
	for (const auto& e : chosen->entries)
	{
		EncounterSpawnEntry entry;
		entry.monsterTypeId = e.monsterTypeId;
		entry.count = e.count;
		entry.level = e.level;
		spawnList.push_back(std::move(entry));
	}

	// Save character before transfer
	if (m_pendingCharacter)
	{
		*m_pendingCharacter = std::move(m_character);
	}

	// Set encounter data and push combat state
	CombatEncounterState::s_spawnEntries = std::move(spawnList);
	m_machine.PushState("CombatEncounter");
}

//=============================================================================

void OverworldState::processOverworldTurn() noexcept
{
	// Hunger decreases each step
	m_character.ConsumeHunger(1);
	m_character.TickBuffs();

	if (m_character.GetHunger() <= 0)
	{
		Logger::Warn("Overworld: starving! Taking damage.");
		m_character.TakeDamage(1);
	}

	if (m_character.GetHp() <= 0)
	{
		Logger::Info("Overworld: character died of starvation.");
		m_machine.ReplaceState("MainMenu");
	}
}

//=============================================================================

void OverworldState::OnEnter() noexcept
{
	try
	{
		Logger::Info("OverworldState::OnEnter begin");
		m_input.ResetState();

		// ---- Load character ----
		if (m_pendingCharacter && m_pendingCharacter->GetLevel() > 0)
		{
			m_character = std::move(*m_pendingCharacter);
			*m_pendingCharacter = Character{};
			m_hasCharacter = true;
		}
		else
		{
			// Create a default character if none exists
			m_character = Character{};
			m_hasCharacter = true;
		}

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

		// ---- Overworld setup ----
		std::string buf = FileReadString("data/locations.json");
		if (!buf.empty())
		{
			json parsed = json::parse(buf, nullptr, false);
			if (!parsed.is_discarded())
			{
				m_overworld.LoadLocations(parsed);
			}
		}

		m_overworld.GenerateDefaultMap();

		m_overworldRenderer.SetFloorMaterial(m_floorMaterial);
		m_overworldRenderer.SetWallMaterial(m_wallMaterial);
		m_overworldRenderer.SetLocationMaterial(m_locationMaterial);

		// ---- Start position: SW corner ----
		m_camera.SetGridPosition({58, 5, 0}, Direction::North);
		m_camera.SnapToGrid();
		m_overworld.MarkVisited({58, 5, 0});
		m_character.GetPosition() = {58, 5, 0};
		m_character.SetFacing(Direction::North);

		// ---- Config ----
		m_moveRepeatDelay = GetGridMoveRepeatDelayFromConfig();

		// ---- Perspective ----
		m_camera.SetPerspective(60.0f,
			static_cast<float>(m_window.Width()) / static_cast<float>(m_window.Height()),
			0.1f, 200.0f);

		// ---- Load encounters ----
		loadEncounters();

		// ---- Input actions ----
		m_input.BindAction("GridMoveForward",  SDL_SCANCODE_W);
		m_input.BindAction("GridMoveBackward", SDL_SCANCODE_S);
		m_input.BindAction("TurnLeft",         SDL_SCANCODE_A);
		m_input.BindAction("TurnRight",        SDL_SCANCODE_D);
		m_input.BindAction("StrafeLeft",       SDL_SCANCODE_Q);
		m_input.BindAction("StrafeRight",      SDL_SCANCODE_E);
		m_input.BindAction("Action_Interact",  SDL_SCANCODE_SPACE);
		m_input.BindAction("Action_FastTravel", SDL_SCANCODE_T);

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

	// Save character back
	if (m_pendingCharacter && m_hasCharacter)
	{
		*m_pendingCharacter = std::move(m_character);
		m_hasCharacter = false;
	}

	delete m_terrainPalette;
	m_terrainPalette = nullptr;
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

	if (event.type == SDL_EVENT_KEY_DOWN)
	{
		switch (event.key.key)
		{
			case SDLK_M:  m_showMap = !m_showMap; break;
			case SDLK_F1: m_showDebug = !m_showDebug; break;
			case SDLK_T:  m_showFastTravel = !m_showFastTravel; break;
			case SDLK_ESCAPE: m_machine.ReplaceState("MainMenu"); break;
			default: break;
		}
	}

	if (event.type == SDL_EVENT_WINDOW_RESIZED)
	{
		if (event.window.data2 > 0)
		{
			m_camera.SetAspectRatio(
				static_cast<float>(event.window.data1) /
				static_cast<float>(event.window.data2));
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
		m_camera.GetGridPosition().col + delta.y, 0);

	return m_overworld.IsWalkable(target);
}

//=============================================================================

void OverworldState::doGridAction(std::string_view name) noexcept
{
	bool wasMovement = (name != "TurnLeft" && name != "TurnRight");

	if (name == "GridMoveForward")  m_camera.MoveForward();
	else if (name == "GridMoveBackward") m_camera.MoveBackward();
	else if (name == "TurnLeft")    m_camera.TurnLeft();
	else if (name == "TurnRight")   m_camera.TurnRight();
	else if (name == "StrafeLeft")  m_camera.MoveLeft();
	else                            m_camera.MoveRight();

	// Mark new position as visited
	GridPosition newPos = m_camera.GetGridPosition();
	m_overworld.MarkVisited(newPos);
	m_character.GetPosition() = newPos;
	m_character.SetFacing(m_camera.Facing());

	// Movement costs a turn: hunger, encounters, etc.
	if (wasMovement)
	{
		processOverworldTurn();
		triggerRandomEncounter();
	}
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
		glm::ivec2 fwd = DirectionToVec(m_camera.Facing());
		GridPosition front = {pos.row + fwd.x, pos.col + fwd.y, 0};
		loc = m_overworld.FindLocationAt(front);
	}

	if (loc)
	{
		Logger::Info(std::string("Interacting with location: ") + loc->name);

		// Save character for transition
		if (m_pendingCharacter)
		{
			*m_pendingCharacter = std::move(m_character);
		}

		// For now, just log. Future: transition to location state.
	}
}

//=============================================================================

void OverworldState::RenderScene(Renderer& renderer) noexcept
{
	if (!m_initialized) return;

	m_renderer = &renderer;

	// Pass camera position and view radius for fog-of-war + culling
	GridPosition camPos = m_camera.GetGridPosition();
	m_overworldRenderer.Build(m_overworld, camPos, m_viewRadius);

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
	ImGui::SetNextWindowSize(ImVec2(260, 180), ImGuiCond_FirstUseEver);
	ImGui::Begin("Overworld");
	ImGui::Text("World: %dx%d", OVERWORLD_SIZE, OVERWORLD_SIZE);
	ImGui::Separator();
	ImGui::Text("W/S \tMove Fwd/Back");
	ImGui::Text("A/D \tTurn L/R");
	ImGui::Text("Q/E \tStrafe L/R");
	ImGui::Text("Space\tInteract");
	ImGui::Text("T    \tFast Travel");
	ImGui::Separator();
	ImGui::Text("M: Map  F1: Debug  Esc: Menu");

	if (m_hasCharacter)
	{
		ImGui::Separator();
		ImGui::Text("HP: %d/%d  MP: %d/%d",
			m_character.GetHp(), m_character.GetMaxHp(),
			m_character.GetMp(), m_character.GetMaxMp());
		ImGui::Text("Hunger: %d/%d", m_character.GetHunger(), Character::MAX_HUNGER);
		ImGui::Text("Lv %d %s", m_character.GetLevel(), m_character.GetClass().c_str());
	}

	// Show current terrain
	GridPosition gp = m_camera.GetGridPosition();
	const OverworldCell& cell = m_overworld.GetCell(gp);
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
	ImGui::SetNextWindowPos(ImVec2(0, 190), ImGuiCond_FirstUseEver, ImVec2(0, 0));
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

	// ---- Fast travel (T key) ----
	if (m_showFastTravel)
	{
		renderFastTravel();
	}

	// ---- Debug ----
	if (m_showDebug)
	{
		ImGui::SetNextWindowPos(ImVec2(0, 250), ImGuiCond_FirstUseEver, ImVec2(0, 0));
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
		ImGui::Text("View Radius: %d", m_viewRadius);
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

//=============================================================================

void OverworldState::renderFastTravel() noexcept
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 ws = viewport->WorkSize;

	ImGui::SetNextWindowPos(ImVec2(ws.x * 0.5f - 200.0f, ws.y * 0.5f - 150.0f),
		ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Fast Travel", &m_showFastTravel))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Select destination (requires road connection):");
	ImGui::Separator();

	GridPosition currentPos = m_camera.GetGridPosition();
	bool anyAvailable = false;

	for (const auto& loc : m_overworld.Locations())
	{
		if (loc.type != LocationType::Town)
		{
			continue;
		}

		GridPosition locPos = {loc.position.row, loc.position.col, 0};
		if (!m_overworld.IsVisited(locPos))
		{
			continue;
		}

		anyAvailable = true;
		bool onRoad = m_overworld.GetCell(currentPos).terrain == TerrainType::Road;

		if (ImGui::Button(loc.name.c_str(), ImVec2(200, 30)))
		{
			if (onRoad)
			{
				m_camera.SetGridPosition(locPos, Direction::North);
				m_camera.SnapToGrid();
				m_overworld.MarkVisited(locPos);
				m_character.GetPosition() = locPos;
				m_showFastTravel = false;
				Logger::Info("Fast travelled to " + loc.name);
			}
		}

		if (onRoad)
		{
			ImGui::SameLine();
			ImGui::Text("(%d, %d) - %s", loc.position.row, loc.position.col,
				loc.description.c_str());
		}
		else
		{
			ImGui::SameLine();
			ImGui::TextDisabled("(need to be on a road)");
		}
	}

	if (!anyAvailable)
	{
		ImGui::Text("No visited towns available for fast travel.");
	}

	ImGui::Separator();
	if (ImGui::Button("Close"))
	{
		m_showFastTravel = false;
	}

	ImGui::End();
}
