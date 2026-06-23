#include "stdafx.h"
#include "game/states/combat_encounter_state.h"
#include "engine/delta_time.h"
#include "engine/input_manager.h"
#include "engine/window.h"
#include "engine/resource_manager.h"
#include "engine/renderer/shader_sources.h"
#include "engine/renderer/renderer.h"
#include "core/logger.h"
#include "core/exception.h"

std::vector<EncounterSpawnEntry> CombatEncounterState::s_spawnEntries;

//=============================================================================

CombatEncounterState::CombatEncounterState(
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
{}

CombatEncounterState::~CombatEncounterState() noexcept = default;

//=============================================================================

void CombatEncounterState::createArenaDungeon() noexcept
{
	Chunk chunk;

	for (int32_t r = 0; r < CHUNK_SIZE; ++r)
	{
		for (int32_t c = 0; c < CHUNK_SIZE; ++c)
		{
			Cell cell;
			cell.hasFloor = true;
			cell.hasCeiling = true;
			chunk.At(r, c) = cell;
		}
	}

	// Perimeter walls
	for (int32_t r = ARENA_ORIGIN_ROW; r < ARENA_ORIGIN_ROW + ARENA_SIZE; ++r)
	{
		Cell& leftWall = chunk.At(r, ARENA_ORIGIN_COL);
		leftWall.isWall = true;
		leftWall.hasFloor = false;

		Cell& rightWall = chunk.At(r, ARENA_ORIGIN_COL + ARENA_SIZE - 1);
		rightWall.isWall = true;
		rightWall.hasFloor = false;
	}
	for (int32_t c = ARENA_ORIGIN_COL; c < ARENA_ORIGIN_COL + ARENA_SIZE; ++c)
	{
		Cell& topWall = chunk.At(ARENA_ORIGIN_ROW, c);
		topWall.isWall = true;
		topWall.hasFloor = false;

		Cell& bottomWall = chunk.At(ARENA_ORIGIN_ROW + ARENA_SIZE - 1, c);
		bottomWall.isWall = true;
		bottomWall.hasFloor = false;
	}

	m_dungeon.SetChunk(chunk);
}

//=============================================================================

void CombatEncounterState::OnEnter() noexcept
{
	s_spawnEntries.clear();

	try
	{
		Logger::Info("CombatEncounterState::OnEnter begin");
		m_input.ResetState();

		// ---- Load character ----
		if (m_pendingCharacter && m_pendingCharacter->GetLevel() > 0)
		{
			m_character = std::move(*m_pendingCharacter);
			*m_pendingCharacter = Character{};
		}
		else
		{
			m_character = Character{};
			Logger::Warn("CombatEncounter: no valid character, using default");
		}

		// ---- Shader ----
		m_dungeonShader = m_resources.GetOrCreateShader(
			"dungeon", DUNGEON_VERT_SRC, DUNGEON_FRAG_SRC);
		if (!m_dungeonShader)
		{
			throw std::runtime_error("Failed to create dungeon shader");
		}

		// ---- Textures ----
		m_texFloor = m_resources.LoadTexture("tex_arena_floor", "data/texture_08.png", false);
		m_texWall  = m_resources.LoadTexture("tex_arena_wall",  "data/texture_10.png", false);
		m_texCeiling = m_resources.LoadTexture("tex_arena_ceiling", "data/texture_11.png", false);

		// ---- Materials ----
		m_matFloor = m_resources.GetOrCreateMaterial("mat_arena_floor", *m_dungeonShader, *m_texFloor);
		m_matWall  = m_resources.GetOrCreateMaterial("mat_arena_wall",  *m_dungeonShader, *m_texWall);
		m_matCeiling = m_resources.GetOrCreateMaterial("mat_arena_ceiling", *m_dungeonShader, *m_texCeiling);

		// ---- Arena dungeon ----
		createArenaDungeon();

		// ---- Position ----
		m_camera.SetGridPosition({PLAYER_START_ROW, PLAYER_START_COL, 0}, Direction::North);
		m_camera.SnapToGrid();
		m_character.GetPosition() = {PLAYER_START_ROW, PLAYER_START_COL, 0};
		m_character.SetFacing(Direction::North);

		// ---- CombatHandler ----
		m_combatHandler.Init(
			m_character, m_monsterManager, m_monsterRenderer, m_combatSystem,
			m_combatLog, m_turnManager, m_camera, m_dungeon, m_resources,
			*m_dungeonShader, m_pendingLevelUp, m_itemHandler
		);
		m_combatHandler.LoadMonsterTextures();
		m_monsterRenderer.Init();

		// ---- Spawn encounter monsters ----
		if (!s_spawnEntries.empty())
		{
			int32_t spawnRow = PLAYER_START_ROW + 2;
			int32_t spawnCol = PLAYER_START_COL;
			for (const auto& entry : s_spawnEntries)
			{
				for (int32_t i = 0; i < entry.count; ++i)
				{
					Monster monster = MonsterFactory::Create(
						entry.monsterTypeId, entry.level);
					monster.position = {spawnRow + (i % 3), spawnCol + (i / 3), 0};
					monster.facing = Direction::South;
					m_monsterManager.Spawn(std::move(monster));
				}
			}
			Logger::Info("CombatEncounter: monsters spawned");
		}
		else
		{
			Monster monster = MonsterFactory::Create("skeleton", 1);
			monster.position = {PLAYER_START_ROW + 2, PLAYER_START_COL, 0};
			monster.facing = Direction::South;
			m_monsterManager.Spawn(std::move(monster));
		}

		// ---- ItemHandler ----
		m_itemHandler.Init(m_character, m_combatLog, m_turnManager, m_camera, m_resources, *m_dungeonShader);

		// ---- SpellSystem ----
		m_spellSystem.Init(m_character, m_monsterManager, m_combatLog, m_dungeon);

		// ---- Input ----
		m_input.BindAction("GridMoveForward",  SDL_SCANCODE_W);
		m_input.BindAction("GridMoveBackward", SDL_SCANCODE_S);
		m_input.BindAction("TurnLeft",         SDL_SCANCODE_A);
		m_input.BindAction("TurnRight",        SDL_SCANCODE_D);
		m_input.BindAction("StrafeLeft",       SDL_SCANCODE_Q);
		m_input.BindAction("StrafeRight",      SDL_SCANCODE_E);
		m_input.BindAction("Action_Attack",    SDL_SCANCODE_SPACE);
		m_input.BindAction("Action_Ranged",    SDL_SCANCODE_R);

		// ---- Perspective ----
		m_camera.SetPerspective(60.0f,
			static_cast<float>(m_window.Width()) / static_cast<float>(m_window.Height()),
			0.1f, 100.0f);

		m_initialized = true;
		Logger::Info("CombatEncounterState initialized");
	}
	catch (const std::exception& e)
	{
		Logger::Error(std::string("CombatEncounterState init failed: ") + e.what());
		m_initialized = false;
	}
}

//=============================================================================

void CombatEncounterState::OnExit() noexcept
{
	m_input.ClearActions();
	m_input.SetMouseCaptured(false);

	if (m_pendingCharacter)
	{
		*m_pendingCharacter = std::move(m_character);
	}
	m_combatLog.Clear();
	m_monsterManager.RemoveDead();
	m_resources.CleanupUnused();
}

void CombatEncounterState::OnPause() noexcept
{
	m_input.SetMouseCaptured(false);
}

void CombatEncounterState::OnResume() noexcept {}

//=============================================================================

void CombatEncounterState::HandleEvent(const SDL_Event& event) noexcept
{
	if (!m_initialized) return;

	if (event.type == SDL_EVENT_KEY_DOWN)
	{
		switch (event.key.key)
		{
			case SDLK_F1: m_showDebug = !m_showDebug; break;
			case SDLK_ESCAPE: m_encounterDone = true; break;
			default: break;
		}
	}

	if (event.type == SDL_EVENT_WINDOW_RESIZED && event.window.data2 > 0)
	{
		m_camera.SetAspectRatio(
			static_cast<float>(event.window.data1) /
			static_cast<float>(event.window.data2));
	}
}

//=============================================================================

void CombatEncounterState::FixedUpdate(float fixedDt) noexcept
{
	(void)fixedDt;
}

//=============================================================================

void CombatEncounterState::Update(const DeltaTime& dt) noexcept
{
	if (!m_initialized) return;
	if (m_encounterDone) return;

	GameMode mode = m_turnManager.GetGameMode();

	if (mode == GameMode::TurnWaiting)
	{
		processTurnWaiting();
	}

	if (mode == GameMode::Exploring || mode == GameMode::TurnAnimating)
	{
		processEdgeActions();
		processAttack();
		processHeldRepeat(dt);
	}

	if (mode == GameMode::TurnAnimating && !m_camera.IsAnimating())
	{
		m_turnManager.SetGameMode(GameMode::Exploring);
	}

	m_camera.UpdateAnimation(static_cast<float>(dt.Seconds()));

	checkEncounterResult();
}

//=============================================================================

void CombatEncounterState::processEdgeActions() noexcept
{
	for (std::string_view name : GRID_ACTION_NAMES)
	{
		if (!m_input.IsActionPressed(name)) continue;

		if (m_camera.IsAnimating())
		{
			m_camera.SnapToGrid();
		}

		bool movement = (name != "TurnLeft" && name != "TurnRight");
		if (movement && !isWalkableAction(name)) break;

		doGridAction(name);
		m_turnManager.SetGameMode(GameMode::TurnWaiting);
		break;
	}
}

//=============================================================================

void CombatEncounterState::processHeldRepeat(const DeltaTime& dt) noexcept
{
	if (m_turnManager.GetGameMode() != GameMode::Exploring) return;
	if (m_camera.IsAnimating()) return;

	for (std::string_view name : GRID_ACTION_NAMES)
	{
		if (m_input.IsActionPressed(name)) return;
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
	if (heldAction.empty()) return;

	float moveRepeatDelay = 0.1f;
	m_repeatTimer += static_cast<float>(dt.Seconds());
	if (m_repeatTimer >= moveRepeatDelay)
	{
		bool movement = (heldAction != "TurnLeft" && heldAction != "TurnRight");
		if (!movement || isWalkableAction(heldAction))
		{
			doGridAction(heldAction);
			m_turnManager.SetGameMode(GameMode::TurnWaiting);
		}
		m_repeatTimer = 0.0f;
	}
}

//=============================================================================

void CombatEncounterState::processAttack() noexcept
{
	if (m_camera.IsAnimating()) return;
	if (m_turnManager.GetGameMode() == GameMode::TurnWaiting) return;

	if (m_input.IsActionPressed("Action_Ranged"))
	{
		m_combatHandler.PerformRangedAttack();
		m_turnManager.SetGameMode(GameMode::TurnWaiting);
		return;
	}

	if (m_input.IsActionPressed("Action_Attack"))
	{
		Monster* target = m_monsterManager.FindInFront(
			m_camera.GetGridPosition(), m_camera.Facing());
		if (target)
		{
			m_combatHandler.PerformCombat();
		}
		m_turnManager.SetGameMode(GameMode::TurnWaiting);
	}
}

//=============================================================================

void CombatEncounterState::processTurnWaiting() noexcept
{
	m_turnManager.ApplyRegen(m_character);

	// Player status effects
	{
		auto& playerEffects = m_character.GetActiveEffects();
		std::vector<std::string> expired;
		m_spellSystem.GetStatusSystem().ProcessTick(
			playerEffects, expired,
			[this](int32_t dmg, const std::string& name)
			{
				m_character.TakeDamage(dmg);
				m_combatLog.Add("Status effect " + name + " deals " +
					std::to_string(dmg) + " damage.", glm::vec3(1.0f, 0.2f, 0.2f));
			}
		);
		m_spellSystem.GetStatusSystem().AdvanceTurns(playerEffects);
	}

	// Monster status effects
	m_monsterManager.ProcessMonsterEffects(m_combatHandler.GetStatusSystem(), m_combatLog);

	// Hunger
	m_character.ConsumeHunger(1);
	m_character.TickBuffs();

	// Monster turns
	m_combatHandler.ProcessMonsterTurns();
	m_monsterManager.RemoveDead();

	m_turnManager.EndPlayerTurn(m_camera.IsAnimating());
}

//=============================================================================

bool CombatEncounterState::isWalkableAction(std::string_view name) const noexcept
{
	if (name == "TurnLeft" || name == "TurnRight") return true;

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

	return m_dungeon.IsWalkable(target)
		&& m_monsterManager.At(target) == nullptr;
}

//=============================================================================

void CombatEncounterState::doGridAction(std::string_view name) noexcept
{
	if (name == "GridMoveForward")  m_camera.MoveForward();
	else if (name == "GridMoveBackward") m_camera.MoveBackward();
	else if (name == "TurnLeft")    m_camera.TurnLeft();
	else if (name == "TurnRight")   m_camera.TurnRight();
	else if (name == "StrafeLeft")  m_camera.MoveLeft();
	else                            m_camera.MoveRight();

	m_character.GetPosition() = m_camera.GetGridPosition();
	m_character.SetFacing(m_camera.Facing());
}

//=============================================================================

void CombatEncounterState::checkEncounterResult() noexcept
{
	bool allDead = true;
	for (const auto& monster : m_monsterManager.All())
	{
		if (monster.alive)
		{
			allDead = false;
			break;
		}
	}

	if (allDead)
	{
		m_showVictory = true;
		m_encounterDone = true;
		return;
	}

	if (m_character.GetHp() <= 0)
	{
		m_showDefeat = true;
		m_encounterDone = true;
	}
}

//=============================================================================

void CombatEncounterState::RenderScene(Renderer& renderer) noexcept
{
	if (!m_initialized) return;

	renderer.BeginFrame(m_camera.ViewMatrix(), m_camera.ProjectionMatrix());
	m_combatHandler.SubmitRender(renderer);
	m_itemHandler.SubmitRender(renderer);
	renderer.EndFrame();
}

//=============================================================================

void CombatEncounterState::RenderOverlay(const glm::mat4& viewProj) noexcept
{
	(void)viewProj;
}

//=============================================================================

void CombatEncounterState::Render() noexcept
{
	if (!m_initialized)
	{
		ImGui::Begin("Error");
		ImGui::Text("Failed to initialize CombatEncounterState.");
		ImGui::End();
		return;
	}

	if (m_showVictory)
	{
		ImGui::OpenPopup("Victory");
	}
	if (m_showDefeat)
	{
		ImGui::OpenPopup("Defeat");
	}

	if (ImGui::BeginPopupModal("Victory", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("All monsters defeated!");
		ImGui::Separator();
		ImGui::Text("XP: %d", m_character.GetXp());
		ImGui::Text("Level: %d", m_character.GetLevel());
		if (m_pendingLevelUp)
		{
			ImGui::TextColored(ImVec4(0, 1, 0, 1), "Level Up!");
		}
		if (ImGui::Button("Return to Overworld"))
		{
			ImGui::CloseCurrentPopup();
			m_machine.PopState();
		}
		ImGui::EndPopup();
	}

	if (ImGui::BeginPopupModal("Defeat", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::Text("You have been defeated!");
		if (ImGui::Button("Back to Main Menu"))
		{
			ImGui::CloseCurrentPopup();
			m_machine.ReplaceState("MainMenu");
		}
		ImGui::EndPopup();
	}

	// ---- Hero HUD ----
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 ws = viewport->WorkSize;

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(260, 110), ImGuiCond_FirstUseEver);
	ImGui::Begin("Hero");
	ImGui::Text("HP");
	ImGui::ProgressBar(
		static_cast<float>(m_character.GetHp()) /
		static_cast<float>(m_character.GetMaxHp()),
		ImVec2(-1, 0));
	ImGui::Text("MP");
	ImGui::ProgressBar(
		static_cast<float>(m_character.GetMp()) /
		static_cast<float>(m_character.GetMaxMp()),
		ImVec2(-1, 0));
	ImGui::Text("AC %d | ATK %+d | DMG %d-%d",
		m_character.GetAc(),
		m_character.GetAtkBonus(),
		m_character.GetDamageMin(),
		m_character.GetDamageMax());
	ImGui::Text("Hunger: %d / %d", m_character.GetHunger(), Character::MAX_HUNGER);
	ImGui::End();

	// ---- Monster Ahead ----
	Monster* target = m_monsterManager.FindInFront(
		m_camera.GetGridPosition(), m_camera.Facing());
	if (target && target->alive)
	{
		ImGui::SetNextWindowPos(ImVec2(0, 120), ImGuiCond_FirstUseEver, ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(260, 70), ImGuiCond_FirstUseEver);
		ImGui::Begin("Monster Ahead");
		ImGui::Text("%s (HP %d/%d)", target->name.c_str(),
			target->hp, target->maxHp);
		ImGui::ProgressBar(
			static_cast<float>(target->hp) /
			static_cast<float>(target->maxHp),
			ImVec2(-1, 0));
		ImGui::End();
	}

	// ---- Combat Log ----
	ImGui::SetNextWindowPos(ImVec2(ws.x - 300, 0), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(300, ws.y), ImGuiCond_FirstUseEver);
	ImGui::Begin("Combat Log");
	if (ImGui::Button("Clear"))
	{
		m_combatLog.Clear();
	}
	ImGui::Separator();
	ImGui::BeginChild("LogEntries");
	const auto& entries = m_combatLog.Entries();
	for (size_t i = 0; i < entries.size(); ++i)
	{
		ImGui::TextColored(
			ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "%s", entries[i].text.c_str());
	}
	if (!entries.empty())
	{
		ImGui::SetScrollHereY(1.0f);
	}
	ImGui::EndChild();
	ImGui::End();

	// ---- Debug ----
	if (m_showDebug)
	{
		ImGui::Begin("Combat Debug");
		ImGui::Text("Mode: %d", static_cast<int>(m_turnManager.GetGameMode()));
		ImGui::Text("Monsters: %zu", m_monsterManager.All().size());
		ImGui::Text("Cam: (%d, %d)", m_camera.GetGridPosition().row,
			m_camera.GetGridPosition().col);
		ImGui::End();
	}
}
