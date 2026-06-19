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
#include "game/serialization.h"
#include "game/save_manager.h"
#include "core/file_io.h"
#include "game/data/experience_system.h"
#include "game/data/skill_manager.h"
#include "game/data/item_factory.h"
#include "game/states/class_selection_state.h"
#include "game/combat/dice.h"

//=============================================================================

PlayState::PlayState(
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
		m_turnManager.SetGameMode(GameMode::Exploring);

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
		m_renderHeight = GetRenderHeightFromConfig();

		// ---- Grid camera ----
		m_camera.SetGridPosition({17, 17, 0}, Direction::North);
		m_camera.SnapToGrid();

		// ---- Character (from class selection or default) ----
		if (m_pendingCharacter && m_pendingCharacter->GetLevel() > 0)
		{
			m_character = std::move(*m_pendingCharacter);
			*m_pendingCharacter = Character{};
		}
		else
		{
			m_character = Character{};
		}
		m_character.GetPosition() = {17, 17, 0};
		m_character.SetFacing(Direction::North);
		Logger::Info("PlayState: character created");

		// ---- Combat handler init ----
		m_combatHandler.Init(
			m_character,
			m_monsterManager,
			m_monsterRenderer,
			m_combatSystem,
			m_combatLog,
			m_turnManager,
			m_camera,
			m_dungeon,
			m_resources,
			*m_dungeonShader,
			m_pendingLevelUp,
			m_itemHandler
		);
		m_combatHandler.LoadMonsterTextures();
		m_monsterRenderer.Init();
		m_combatHandler.SpawnDefault();
		Logger::Info("PlayState: monsters spawned");

		// ---- Item handler init ----
		m_itemHandler.Init(
			m_character,
			m_combatLog,
			m_turnManager,
			m_camera,
			m_resources,
			*m_dungeonShader
		);

		m_showInventory = false;

		Logger::Info("PlayState: before SpawnDefault");
		m_itemHandler.SpawnDefault();
		Logger::Info("PlayState: after SpawnDefault");
		Logger::Info("PlayState: item drops spawned");

		// ---- Spell system init ----
		Logger::Info("PlayState: before spell system Init");
		m_spellSystem.Init(
			m_character,
			m_monsterManager,
			m_combatLog,
			m_dungeon
		);
		Logger::Info("PlayState: after spell system Init");
		Logger::Info("PlayState: spell system initialized");

		// ---- Encounter manager init ----
		Logger::Info("PlayState: before LoadEncounters");
		m_encounterManager.LoadEncounters();
		Logger::Info("PlayState: after LoadEncounters");
		Logger::Info("PlayState: encounter manager initialized");

		// ---- Crafting system init (steps 166-173) ----
		Logger::Info("PlayState: before CraftingSystem init");
		m_craftingSystem.LoadRecipes();
		m_craftingSystem.LoadCategories();
		m_craftingSystem.UnlockAllRecipes(); // step 173: all recipes unlocked by default
		Logger::Info("PlayState: after CraftingSystem init");

		// ---- Input actions for grid movement ----
		m_input.BindAction("GridMoveForward",  SDL_SCANCODE_W);
		m_input.BindAction("GridMoveBackward", SDL_SCANCODE_S);
		m_input.BindAction("TurnLeft",         SDL_SCANCODE_A);
		m_input.BindAction("TurnRight",        SDL_SCANCODE_D);
		m_input.BindAction("StrafeLeft",       SDL_SCANCODE_Q);
		m_input.BindAction("StrafeRight",      SDL_SCANCODE_E);
		m_input.BindAction("Action_Attack",    SDL_SCANCODE_SPACE);
		m_input.BindAction("Action_Search",    SDL_SCANCODE_F);
		m_input.BindAction("Action_Ranged",    SDL_SCANCODE_R);

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
		m_showMap = !m_showMap;
	}

	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F2)
	{
		m_showOptions = !m_showOptions;
	}

	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_F1)
	{
		if (m_showDebug)
		{
			exitDebugMode();
			m_showDebug = false;
		}
		else
		{
			enterDebugMode();
			m_showDebug = true;
		}
	}

	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_I)
	{
		m_showInventory = !m_showInventory;
	}

	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_K)
	{
		m_showSkills = !m_showSkills;
	}

	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_E)
	{
		m_showEquipment = !m_showEquipment;
	}

	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_B)
	{
		m_showSpellbook = !m_showSpellbook;
	}

	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_U)
	{
		m_showStatusEffects = !m_showStatusEffects;
	}

	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_C)
	{
		m_showCrafting = !m_showCrafting;
	}

	// Spell targeting: Space confirms target
	if (m_targetingActive && event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_SPACE)
	{
		ProcessSpellAction();
	}

	// Escape cancels targeting
	if (m_targetingActive && event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_ESCAPE)
	{
		m_targetingActive = false;
		m_targetingSpellId.clear();
		m_wandItemSpellId.clear();
		m_targetingModeActive = false;
		m_showTargetingPreview = false;
		m_targetPreview = SpellTarget{};
		m_debugRenderer.ClearOverlay();
		m_combatLog.Add("Spell cancelled.", glm::vec3(0.6f));
	}

	// Hotbar 1-9
	if (event.type == SDL_EVENT_KEY_DOWN)
	{
		SDL_Keycode k = event.key.key;
		if (k >= SDLK_1 && k <= SDLK_9)
		{
			int32_t slotIdx = static_cast<int32_t>(k - SDLK_1);
			const auto& slot = m_character.GetActionSlots()[slotIdx];
			if (slot.type == "spell")
			{
				// Spells go through targeting system
				m_targetingActive = true;
				m_targetingSpellId = slot.id;
				Skill spell = SkillManager::GetSkill(slot.id);
				m_combatLog.Add("Select target for " + spell.name + " (Space to cast, Esc to cancel).",
					glm::vec3(0.6f, 0.8f, 1.0f));
			}
			else
			{
				m_combatHandler.ProcessActionSlot(slotIdx);
			}
		}
	}

	if (event.type == SDL_EVENT_KEY_DOWN)
	{
		if (event.key.key == SDLK_F5)
		{
			SaveGame("save.json");
		}
		else if (event.key.key == SDLK_F9)
		{
			LoadGame("save.json");
		}
		else if (event.key.key == SDLK_M)
		{
			m_showMap = !m_showMap;
		}
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

	if (m_turnManager.GetGameMode() == GameMode::GameOver)
	{
		// No input processing during GameOver
		m_camera.UpdateAnimation(static_cast<float>(dt.Seconds()));
		m_audio.Update();
		return;
	}

	if (m_pendingLevelUp)
	{
		m_showLevelUp = true;
		m_pendingLevelUp = false;
	}

	if (m_showLevelUp)
	{
		m_camera.UpdateAnimation(static_cast<float>(dt.Seconds()));
		m_audio.Update();
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
		processAttack();
		processHeldRepeat(dt);

	// If animation finished while in TurnAnimating, go back to Exploring
		if (m_turnManager.GetGameMode() == GameMode::TurnAnimating && !m_camera.IsAnimating())
		{
			m_turnManager.SetGameMode(GameMode::Exploring);
		}
	}

	// Update AoE targeting preview (step 141)
	updateTargetingPreview();

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
	if (!m_dungeon.IsWalkable(target))
	{
		return false;
	}
	// Block movement if a monster occupies the target cell
	if (m_monsterManager.At(target) != nullptr)
	{
		return false;
	}
	return true;
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

	// Sync character state with camera after every grid action
	m_character.GetPosition() = m_camera.GetGridPosition();
	m_character.SetFacing(m_camera.Facing());
}

//=============================================================================

void PlayState::processEdgeActions() noexcept
{
	// Edge-triggered actions are processed during Exploring and TurnAnimating.
	// TurnWaiting processes instantly (same frame), so this runs in Exploring
	// the next frame. TurnWaiting itself blocks new input.
	if (m_turnManager.GetGameMode() != GameMode::Exploring && m_turnManager.GetGameMode() != GameMode::TurnAnimating)
	{
		return;
	}

	for (std::string_view name : GRID_ACTION_NAMES)
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
			m_turnManager.SetGameMode(GameMode::TurnWaiting);
		}

		break;
	}
}

//=============================================================================

void PlayState::processHeldRepeat(const DeltaTime& dt) noexcept
{
	// Auto-repeat only during Exploring, when camera is idle
	if (m_turnManager.GetGameMode() != GameMode::Exploring)
	{
		return;
	}
	if (m_camera.IsAnimating())
	{
		return;
	}

	// If any key is freshly pressed (edge), skip repeat this frame
	for (std::string_view name : GRID_ACTION_NAMES)
	{
		if (m_input.IsActionPressed(name))
		{
			return;
		}
	}

	// Find a held key
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
		bool movement = isMovementAction(heldAction);

		// Check walkability for movement actions
		if (!movement || isWalkableAction(heldAction))
		{
			doGridAction(heldAction);

			if (movement)
			{
				m_turnManager.SetGameMode(GameMode::TurnWaiting);
			}
		}
		m_moveRepeatTimer = 0.0f;
	}
}

//=============================================================================

void PlayState::processTurnWaiting() noexcept
{
	if (m_turnManager.GetGameMode() != GameMode::TurnWaiting)
	{
		return;
	}

	m_turnManager.ApplyRegen(m_character);

	// Process status effects on player (step 145)
	{
		auto& playerEffects = m_character.GetActiveEffects();
		std::vector<std::string> expired;
		m_spellSystem.GetStatusSystem().ProcessTick(
			playerEffects,
			expired,
			[this](int32_t dmg, const std::string& name)
			{
				m_character.TakeDamage(dmg);
				m_combatLog.Add("Status effect " + name + " deals " +
					std::to_string(dmg) + " damage.", glm::vec3(1.0f, 0.2f, 0.2f));
			}
		);
		for (const auto& eid : expired)
		{
			m_combatLog.Add("Status effect '" + eid + "' has expired.",
				glm::vec3(0.6f, 0.6f, 0.6f));
		}
		m_spellSystem.GetStatusSystem().AdvanceTurns(playerEffects);
	}

	// Process monster status effects (step 148)
	m_monsterManager.ProcessMonsterEffects(m_combatHandler.GetStatusSystem(), m_combatLog);

	// Hunger system (step 211-217)
	if (m_character.GetHunger() > 0)
	{
		m_character.ConsumeHunger(1);
	}

	int32_t hunger = m_character.GetHunger();

	// Step 212: warning messages at thresholds
	if (hunger < Character::HUNGER_WARNING && hunger >= Character::HUNGER_STARVING && !m_hungerWarningShown)
	{
		m_combatLog.Add("You feel hungry.", glm::vec3(1.0f, 0.8f, 0.2f));
		m_hungerWarningShown = true;
		m_hungerStarvingShown = false;
	}
	if (hunger < Character::HUNGER_STARVING && hunger > 0 && !m_hungerStarvingShown)
	{
		m_combatLog.Add("You are starving! (-1 ATK, -1 AC)", glm::vec3(1.0f, 0.3f, 0.0f));
		m_hungerStarvingShown = true;
	}

	// Step 214: starvation damage
	if (hunger <= 0)
	{
		m_character.TakeDamage(1);
		m_combatLog.Add("You collapse from hunger! (-1 HP)",
			glm::vec3(1.0f, 0.2f, 0.2f));
		if (m_character.GetHp() <= 0)
		{
			m_combatLog.Add("Hero has fallen from starvation!", glm::vec3(1.0f, 0.0f, 0.0f));
			m_turnManager.SetGameMode(GameMode::GameOver);
			return;
		}
	}

	// Reset flags when hunger recovers above threshold
	if (hunger >= Character::HUNGER_WARNING)
	{
		m_hungerWarningShown = false;
		m_hungerStarvingShown = false;
	}

	// Tick food buffs (step 222-224)
	{
		int32_t buffRegen = m_character.TickBuffs();
		if (buffRegen > 0)
		{
			m_character.Heal(buffRegen);
		}
	}

	// Process monster combat turns (all monsters attack)
	m_combatHandler.ProcessMonsterTurns();

	// Remove dead monsters
	m_monsterManager.RemoveDead();

	m_turnManager.EndPlayerTurn(m_camera.IsAnimating());
}

//=============================================================================

void PlayState::processAttack() noexcept
{
	if (m_turnManager.GetGameMode() != GameMode::Exploring && m_turnManager.GetGameMode() != GameMode::TurnAnimating)
	{
		return;
	}

	// Search action (F key)
	if (m_input.IsActionPressed("Action_Search"))
	{
		processSearch();
		return;
	}

	// Ranged attack (R key)
	if (m_input.IsActionPressed("Action_Ranged"))
	{
		if (m_combatHandler.HasRangedWeaponEquipped())
		{
			m_combatHandler.PerformRangedAttack();
		}
		else
		{
			m_combatLog.Add("No ranged weapon equipped.", glm::vec3(0.6f));
		}
		return;
	}

	if (!m_input.IsActionPressed("Action_Attack"))
	{
		return;
	}

	// Context-sensitive: monster in front → attack; resource node → gather; else → pickup
	Monster* target = m_monsterManager.FindInFront(
		m_camera.GetGridPosition(),
		m_camera.Facing()
	);

	if (target)
	{
		m_combatHandler.PerformCombat();
	}
	else if (processGather())
	{
		// Gathering handled
	}
	else
	{
		m_itemHandler.ProcessPickup();
	}
}

//=============================================================================



//=============================================================================

void PlayState::renderInventoryWindow() noexcept
{
	if (!m_showInventory)
	{
		return;
	}

	ImGui::SetNextWindowPos(ImVec2(
		ImGui::GetMainViewport()->WorkSize.x * 0.5f - 150.0f,
		ImGui::GetMainViewport()->WorkSize.y * 0.5f - 150.0f
	), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(300, 300), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Inventory", &m_showInventory))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("Items: %zu / %zu", m_character.GetInventory().Size(), Inventory::MAX_ITEMS);
	ImGui::Separator();

	if (m_character.GetInventory().Size() == 0)
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(empty)");
	}
	else
	{
		int32_t i = 0;
		while (i < static_cast<int32_t>(m_character.GetInventory().Size()))
		{
			const Item* item = m_character.GetInventory().Get(i);
			if (!item)
			{
				++i;
				continue;
			}

			ImGui::PushID(i);
			ImGui::Text("%s", item->name.c_str());

			bool removed = false;

			if (item->slot != EquipmentSlot::None)
			{
				ImGui::SameLine();
				if (ImGui::SmallButton("Equip"))
				{
					std::optional<Item> prev = m_character.GetEquipment().Equip(*item);
					m_character.GetInventory().Remove(i);
					if (prev.has_value())
					{
						m_character.GetInventory().Add(std::move(prev.value()));
					}
					removed = true;
				}
			}

			bool isUsableConsumable = (item->type == ItemType::PotionHeal
				|| item->type == ItemType::PotionMana
				|| item->type == ItemType::Scroll
				|| item->type == ItemType::SpellScroll
				|| item->type == ItemType::Wand
				|| item->type == ItemType::Food);

			if (!removed && isUsableConsumable)
			{
				ImGui::SameLine();
				if (ImGui::SmallButton("Use"))
				{
					if (item->type == ItemType::PotionHeal)
					{
						m_character.Heal(item->value);
						m_combatLog.Add(
							"Used " + item->name + " (healed " + std::to_string(item->value) + " HP)!",
							glm::vec3(0.2f, 1.0f, 0.2f));
						m_character.GetInventory().Remove(i);
						removed = true;
					}
					else if (item->type == ItemType::PotionMana)
					{
						int32_t restore = (item->value > 0) ? item->value : 10;
						m_character.RestoreMp(restore);
						m_combatLog.Add(
							"Used " + item->name + " (restored " + std::to_string(restore) + " MP)!",
							glm::vec3(0.2f, 0.4f, 1.0f));
						m_character.GetInventory().Remove(i);
						removed = true;
					}
					else if (item->type == ItemType::SpellScroll)
					{
						if (!item->spellId.empty())
						{
							// Learn the spell
							auto& learned = m_character.GetLearnedSpells();
							bool alreadyKnown = false;
							for (const auto& sid : learned)
							{
								if (sid == item->spellId) { alreadyKnown = true; break; }
							}
							if (!alreadyKnown)
							{
								learned.push_back(item->spellId);
								m_combatLog.Add("Learned spell: " + item->name + "!",
									glm::vec3(0.4f, 0.8f, 1.0f));
							}
							else
							{
								m_combatLog.Add("Already know this spell.",
									glm::vec3(0.6f));
							}
						}
						m_character.GetInventory().Remove(i);
						removed = true;
					}
					else if (item->type == ItemType::Wand)
					{
						if (item->charges > 0 && !item->spellId.empty())
						{
							Skill wandSpell = SkillManager::GetSkill(item->spellId);
							if (wandSpell.type == "self" || wandSpell.type == "heal")
							{
								// Self-target: cast immediately
								m_wandItemSpellId = item->spellId;
								SpellTarget selfTarget;
								selfTarget.position = m_character.GetPosition();
								m_spellSystem.ApplySpell(wandSpell, selfTarget);
								auto* inv = &m_character.GetInventory();
								for (size_t wi = 0; wi < inv->Size(); ++wi)
								{
									const Item* wi2 = inv->Get(wi);
									if (wi2 && wi2->type == ItemType::Wand && wi2->spellId == item->spellId)
									{
										Item modified = *wi2;
										modified.charges--;
										inv->Remove(wi);
										if (modified.charges > 0)
										{
											inv->Add(std::move(modified));
										}
										else
										{
											m_combatLog.Add("Wand crumbles to dust!", glm::vec3(0.8f, 0.6f, 0.2f));
										}
										removed = true;
										break;
									}
								}
								m_wandItemSpellId.clear();
							}
							else
							{
								m_wandItemSpellId = item->spellId;
								m_targetingActive = true;
								m_targetingSpellId = item->spellId;
								m_combatLog.Add("Wand: select target (Space to cast, Esc to cancel).",
									glm::vec3(0.6f, 0.8f, 1.0f));
							}
						}
						else
						{
							m_combatLog.Add("Wand is out of charges!",
								glm::vec3(0.8f, 0.4f, 0.2f));
						}
					}
					else if (item->type == ItemType::Food)
					{
						// Step 215: Food restores hunger
						const json& baseData = JsonDataManager::Instance().GetItemBaseData(item->itemId);
						int32_t hungerRestore = baseData.value("hungerRestore", 0);
						std::string subtype = baseData.value("subtype", "");

						m_character.AddHunger(hungerRestore);
						m_combatLog.Add(
							"Eat " + item->name + " (+" + std::to_string(hungerRestore) + " hunger).",
							glm::vec3(0.8f, 0.6f, 0.2f));

						// Step 218: Raw meat — 30% chance of poison
						if (subtype == "raw_meat")
						{
							if (Dice::Roll(1, 100) <= 30)
							{
								m_spellSystem.GetStatusSystem().ApplyEffect(
									m_character.GetActiveEffects(),
									"poison",
									"Raw Meat",
									3
								);
								m_combatLog.Add("Raw meat gives you food poisoning!",
									glm::vec3(1.0f, 0.2f, 0.2f));
							}
						}
						// Step 219: Cooked food gives buffs
						else if (subtype == "cooked_meat")
						{
							Character::FoodBuff buff;
							buff.id = "cooked_meat_buff";
							buff.name = "Well Fed";
							buff.atkBonus = 1;
							buff.remainingTurns = 20;
							m_character.ApplyFoodBuff(buff);
							m_combatLog.Add("Well Fed: +1 ATK for 20 turns.",
								glm::vec3(0.2f, 1.0f, 0.2f));
						}
						else if (subtype == "soup")
						{
							Character::FoodBuff buff;
							buff.id = "soup_buff";
							buff.name = "Warm Soup";
							buff.mpRegen = 1;
							buff.remainingTurns = 30;
							m_character.ApplyFoodBuff(buff);
							m_combatLog.Add("Warm Soup: +1 MP regen for 30 turns.",
								glm::vec3(0.2f, 0.6f, 1.0f));
						}
						else if (subtype == "meal")
						{
							Character::FoodBuff buff;
							buff.id = "meal_buff";
							buff.name = "Full Meal";
							buff.atkBonus = 2;
							buff.acBonus = 2;
							buff.remainingTurns = 30;
							m_character.ApplyFoodBuff(buff);
							m_combatLog.Add("Full Meal: +2 ATK, +2 AC for 30 turns.",
								glm::vec3(0.2f, 1.0f, 0.2f));
						}
						else if (subtype == "drink")
						{
							// Alcohol: temporary +1 ATK, -1 AC (step 225)
							Character::FoodBuff buff;
							buff.id = "drink_buff";
							buff.name = "Tipsy";
							buff.atkBonus = 1;
							buff.acBonus = -1;
							buff.remainingTurns = 15;
							m_character.ApplyFoodBuff(buff);
							m_combatLog.Add("Tipsy: +1 ATK, -1 AC for 15 turns.",
								glm::vec3(1.0f, 0.6f, 0.2f));
						}

						m_character.GetInventory().Remove(i);
						removed = true;
					}
					else if (item->type == ItemType::Scroll)
					{
						m_combatLog.Add("This scroll has no effect.",
							glm::vec3(0.6f));
					}
				}
			}

			if (!removed)
			{
				ImGui::SameLine();
				if (ImGui::SmallButton("Drop"))
				{
					ItemDrop drop;
					drop.item = *item;
					drop.position = m_camera.GetGridPosition();
					m_itemHandler.Drops().push_back(std::move(drop));
					m_combatLog.Add("Dropped " + item->name + ".", glm::vec3(0.6f));
					m_character.GetInventory().Remove(i);
					removed = true;
				}
			}

			ImGui::PopID();

			if (!removed)
			{
				++i;
			}
		}
	}

	ImGui::End();
}

//=============================================================================

void PlayState::SaveGame(const char* path) noexcept
{
	SaveManager::SaveGame(path, m_character, m_dungeon, m_monsterManager, m_itemHandler.Drops(), m_combatLog);
}

//=============================================================================

void PlayState::LoadGame(const char* path) noexcept
{
	if (SaveManager::LoadGame(path, m_character, m_dungeon, m_monsterManager, m_itemHandler.Drops(), m_camera, m_dungeonRenderer, m_combatLog))
	{
		m_turnManager.SetGameMode(GameMode::Exploring);
	}
}

//=============================================================================

void PlayState::renderMapWindow() noexcept
{
	if (!m_showMap && !m_showDebug)
	{
		return;
	}

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 ws = viewport->WorkSize;

	ImGui::SetNextWindowPos(ImVec2(ws.x * 0.5f - 275.0f, 50.0f), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(550, 550), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Map", &m_showMap, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
	{
		ImGui::End();
		return;
	}

	const int32_t chunkSize = m_dungeon.ChunkSize();
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const float cellSize = (std::min)(avail.x, avail.y) / static_cast<float>(chunkSize);
	const float mapSize = cellSize * static_cast<float>(chunkSize);

	// Center the map in the available area
	const float offsetX = (avail.x - mapSize) * 0.5f;
	const float offsetY = (avail.y - mapSize) * 0.5f;

	ImDrawList* draw = ImGui::GetWindowDrawList();
	const ImVec2 origin = ImGui::GetCursorScreenPos() + ImVec2(offsetX, offsetY);

	// Draw cells
	for (int32_t r = 0; r < chunkSize; ++r)
	{
		for (int32_t c = 0; c < chunkSize; ++c)
		{
			const Cell& cell = m_dungeon.GetCell({r, c, 0});
			ImVec2 p0 = origin + ImVec2(static_cast<float>(c) * cellSize, static_cast<float>(r) * cellSize);
			ImVec2 p1 = p0 + ImVec2(cellSize, cellSize);

			uint32_t color;
			if (cell.isWall)
			{
				color = IM_COL32(60, 50, 40, 255);
			}
			else if (cell.hasFloor)
			{
				color = IM_COL32(30, 30, 30, 255);
			}
			else
			{
				color = IM_COL32(10, 10, 10, 255);
			}

			draw->AddRectFilled(p0, p1, color);
		}
	}

	// Draw items as small blue dots
	for (const ItemDrop& drop : m_itemHandler.Drops())
	{
		ImVec2 center = origin + ImVec2(
			(static_cast<float>(drop.position.col) + 0.5f) * cellSize,
			(static_cast<float>(drop.position.row) + 0.5f) * cellSize);
		float radius = cellSize * 0.25f;
		draw->AddCircleFilled(center, radius, IM_COL32(100, 100, 255, 200), 8);
	}

	// Draw monsters as red dots
	for (const Monster& mon : m_monsterManager.All())
	{
		if (!mon.alive)
		{
			continue;
		}
		ImVec2 center = origin + ImVec2(
			(static_cast<float>(mon.position.col) + 0.5f) * cellSize,
			(static_cast<float>(mon.position.row) + 0.5f) * cellSize);
		float radius = cellSize * 0.3f;
		draw->AddCircleFilled(center, radius, IM_COL32(220, 40, 40, 220), 10);
		// Facing indicator
		glm::ivec2 fwd = DirectionToVec(mon.facing);
		ImVec2 tip = center + ImVec2(
			static_cast<float>(fwd.y) * cellSize * 0.4f,
			static_cast<float>(fwd.x) * cellSize * 0.4f);
		draw->AddLine(center, tip, IM_COL32(255, 100, 100, 220), 2.0f);
	}

	// Draw player as green dot
	{
		GridPosition ppos = m_camera.GetGridPosition();
		ImVec2 center = origin + ImVec2(
			(static_cast<float>(ppos.col) + 0.5f) * cellSize,
			(static_cast<float>(ppos.row) + 0.5f) * cellSize);
		float radius = cellSize * 0.35f;
		draw->AddCircleFilled(center, radius, IM_COL32(40, 220, 40, 240), 12);
		// Facing indicator
		glm::ivec2 fwd = DirectionToVec(m_camera.Facing());
		ImVec2 tip = center + ImVec2(
			static_cast<float>(fwd.y) * cellSize * 0.45f,
			static_cast<float>(fwd.x) * cellSize * 0.45f);
		draw->AddLine(center, tip, IM_COL32(100, 255, 100, 240), 2.5f);
	}

	// Grid lines
	for (int32_t i = 0; i <= chunkSize; ++i)
	{
		float pos = static_cast<float>(i) * cellSize;
		ImVec2 lineH0 = origin + ImVec2(0.0f, pos);
		ImVec2 lineH1 = origin + ImVec2(mapSize, pos);
		draw->AddLine(lineH0, lineH1, IM_COL32(50, 50, 50, 80), 0.5f);

		ImVec2 lineV0 = origin + ImVec2(pos, 0.0f);
		ImVec2 lineV1 = origin + ImVec2(pos, mapSize);
		draw->AddLine(lineV0, lineV1, IM_COL32(50, 50, 50, 80), 0.5f);
	}

	ImGui::Dummy(ImVec2(mapSize, mapSize));
	ImGui::End();
}

//=============================================================================

void PlayState::RestartGame() noexcept
{
	m_turnManager.SetGameMode(GameMode::Exploring);
	m_showInventory = false;
	m_showMap = false;
	m_combatLog.Clear();
	m_pendingLevelUp = false;
	m_showLevelUp = false;

	// Reset character from class data
	m_character = CreateCharacterFromClass(m_character.GetClass());
	m_character.GetPosition() = {17, 17, 0};
	m_character.SetFacing(Direction::North);

	// Reset camera
	m_camera.SetGridPosition({17, 17, 0}, Direction::North);
	m_camera.SnapToGrid();

	// Regenerate dungeon
	m_dungeon.GenerateTestRoom();
	m_dungeonRenderer.SetNeedsRebuild(true);

	// Respawn monsters
	m_monsterManager.Clear();
	m_combatHandler.SpawnDefault();

	// Respawn items
	m_itemHandler.ClearDrops();
	m_itemHandler.SpawnDefault();

	m_moveRepeatDelay = GetGridMoveRepeatDelayFromConfig();
	m_renderHeight = GetRenderHeightFromConfig();

	m_craftingSystem.m_craftingLevel = 1;
	m_craftingSystem.UnlockAllRecipes();

	// Reset hunger state
	m_character.SetHunger(Character::MAX_HUNGER);
	m_hungerWarningShown = false;
	m_hungerStarvingShown = false;
	m_character.GetActiveBuffs().clear();

	m_combatLog.Add("Game restarted.", glm::vec3(0.3f, 0.8f, 1.0f));
}

//=============================================================================

void PlayState::renderOptionsWindow() noexcept
{
	if (!m_showOptions)
	{
		return;
	}

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 ws = viewport->WorkSize;

	ImGui::SetNextWindowPos(ImVec2(ws.x * 0.5f - 200.0f, ws.y * 0.5f - 150.0f), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(400, 300), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Options", &m_showOptions))
	{
		ImGui::End();
		return;
	}

	ImGui::Text("In-Game Settings");
	ImGui::Separator();

	float repeatDelay = m_moveRepeatDelay;
	int rh = m_renderHeight;

	if (ImGui::SliderFloat("Move Repeat Delay", &repeatDelay, 0.05f, 0.5f, "%.2f s"))
	{
		m_moveRepeatDelay = repeatDelay;
	}

	if (ImGui::SliderInt("Render Height", &rh, 120, 1080, "%d px"))
	{
		if (rh < 120) { rh = 120; }
		if (rh > 1080) { rh = 1080; }
	}
	ImGui::TextDisabled("Retro FBO height. Requires restart.");

	ImGui::Separator();

	if (ImGui::Button("Save"))
	{
		// Read current full config, update only our fields
		json j;
		std::string buf = FileReadString("player_config.json");
		if (!buf.empty())
		{
			json parsed = json::parse(buf, nullptr, false);
			if (!parsed.is_discarded())
			{
				j = std::move(parsed);
			}
		}

		j["gridMoveRepeatDelay"] = repeatDelay;
		j["renderHeight"] = rh;

		std::string dumped = j.dump(2);
		if (FileWriteBytes("player_config.json", dumped.data(), dumped.size()))
		{
			m_combatLog.Add("Options saved.", glm::vec3(0.3f, 0.8f, 1.0f));
		}
		else
		{
			m_combatLog.Add("Failed to save options!", glm::vec3(1.0f, 0.3f, 0.0f));
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Cancel"))
	{
		m_showOptions = false;
	}

	ImGui::End();
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

	m_combatHandler.SubmitRender(renderer);

	m_itemHandler.SubmitRender(renderer);

	m_debugRenderer.SetEnabled(m_showDebug);

	renderer.EndFrame();
}

//=============================================================================

void PlayState::RenderOverlay(const glm::mat4& viewProj) noexcept
{
	PROFILE_FUNCTION();

	if (!m_initialized)
	{
		return;
	}

	// Draw AoE targeting grid overlay (always visible)
	if (m_showTargetingPreview)
	{
		renderAoeGridOverlay();
	}

	// Flush overlay AND debug lines
	m_debugRenderer.SetViewProj(viewProj);
	m_debugRenderer.Flush(m_camera.ViewMatrix(), m_camera.ProjectionMatrix());
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

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 ws = viewport->WorkSize;

	// ===== Left panel: controls + HUD =====

	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(260, 140), ImGuiCond_FirstUseEver);
	ImGui::Begin("Dungeon Crawler");
	const int32_t chunkSize = m_dungeon.ChunkSize();
	ImGui::Text("Chunk: %dx%d", chunkSize, chunkSize);
	ImGui::Separator();
	ImGui::Text("W/S \tMove Fwd/Back");
	ImGui::Text("A/D \tTurn L/R");
	ImGui::Text("Q/E \tStrafe L/R");
	ImGui::Text("Space\tAttack (melee)");
	ImGui::Text("R\tRanged Attack");
	ImGui::Text("F\tSearch");
	ImGui::Separator();
	ImGui::Text("Tab: Map [%s]", m_showMap ? "ON" : "OFF");
	ImGui::Text("F1: Debug [%s]", m_showDebug ? "ON" : "OFF");
	ImGui::Text("F2: Options");
	ImGui::Text("F5: Save / F9: Load");
	ImGui::Text("I: Inventory / C: Crafting");
	if (ImGui::Button("Back to Menu"))
	{
		m_machine.ReplaceState("MainMenu");
	}
	ImGui::End();

	// ---- Hero HUD ----
	ImGui::SetNextWindowPos(ImVec2(0, 150), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(260, 110), ImGuiCond_FirstUseEver);
	ImGui::Begin("Hero");
	const float hpFrac = static_cast<float>(m_character.GetHp()) / static_cast<float>(m_character.GetMaxHp());
	ImGui::Text("%s", m_character.GetName().c_str());
	ImGui::ProgressBar(hpFrac, ImVec2(-1.0f, 0.0f),
		(std::to_string(m_character.GetHp()) + " / " + std::to_string(m_character.GetMaxHp()) + " HP").c_str());
	const float mpFrac = (m_character.GetMaxMp() > 0)
		? static_cast<float>(m_character.GetMp()) / static_cast<float>(m_character.GetMaxMp())
		: 0.0f;
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(0.2f, 0.4f, 1.0f, 1.0f));
	ImGui::ProgressBar(mpFrac, ImVec2(-1.0f, 0.0f),
		(m_character.GetMaxMp() > 0
			? (std::to_string(m_character.GetMp()) + " / " + std::to_string(m_character.GetMaxMp()) + " MP").c_str()
			: "No MP"));
	ImGui::PopStyleColor();
	ImGui::Text("AC: %d", m_character.GetEquippedAc());
	ImGui::Text("Attack Bonus: %+d", m_character.GetEquippedAtkBonus());
	ImGui::Text("Damage: %dd%d", m_character.GetEquippedDamageMin(), m_character.GetEquippedDamageMax());
	renderHungerIndicator();
	ImGui::Separator();
	const float xpFrac = (m_character.GetXpForNext() > 0)
		? static_cast<float>(m_character.GetXp()) / static_cast<float>(m_character.GetXpForNext())
		: 0.0f;
	ImGui::ProgressBar(xpFrac, ImVec2(-1.0f, 0.0f),
		(std::to_string(m_character.GetXp()) + " / " + std::to_string(m_character.GetXpForNext()) + " XP").c_str());
	ImGui::End();

	// ---- Monster in front ----
	{
		Monster* front = m_monsterManager.FindInFront(
			m_camera.GetGridPosition(), m_camera.Facing());
		if (front)
		{
			ImGui::SetNextWindowPos(ImVec2(0, 270), ImGuiCond_FirstUseEver, ImVec2(0, 0));
			ImGui::SetNextWindowSize(ImVec2(260, 90), ImGuiCond_FirstUseEver);
			ImGui::Begin("Monster Ahead");
			ImGui::Text("%s", front->name.c_str());
			const float mHpFrac = static_cast<float>(front->hp) / static_cast<float>(front->maxHp);
			ImGui::ProgressBar(mHpFrac, ImVec2(-1.0f, 0.0f),
				(std::to_string(front->hp) + " / " + std::to_string(front->maxHp)).c_str());
			ImGui::Text("AC: %d", front->ac);
			ImGui::End();
		}
	}

	// ===== Right panel: combat log =====

	ImGui::SetNextWindowPos(ImVec2(ws.x - 300, 0), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(300, ws.y), ImGuiCond_FirstUseEver);
	ImGui::Begin("Combat Log");
	if (ImGui::Button("Clear"))
	{
		m_combatLog.Clear();
	}
	ImGui::Separator();
	const float logHeight = ImGui::GetContentRegionAvail().y;
	if (ImGui::BeginChild("LogEntries", ImVec2(0.0f, logHeight), true))
	{
		for (const LogEntry& entry : m_combatLog.Entries())
		{
			ImGui::TextColored(
				ImVec4(entry.color.r, entry.color.g, entry.color.b, 1.0f),
				"%s", entry.text.c_str()
			);
		}
		if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
		{
			ImGui::SetScrollHereY(1.0f);
		}
	}
	ImGui::EndChild();
	ImGui::End();

	// ===== Inventory window (toggle with I) =====
	renderInventoryWindow();

	// ===== Map window (toggle with Tab / M) =====
	renderMapWindow();

	// ===== Options window (toggle with F2) =====
	renderOptionsWindow();

	// ===== Hotbar (always visible) =====
	renderHotbar();

	// ===== Skills window (toggle with K) =====
	renderSkillsWindow();

	// ===== Equipment window (toggle with E) =====
	renderEquipmentWindow();

	// ===== Spellbook window (toggle with B) =====
	renderSpellbookWindow();

	// ===== Status effects window (toggle with U) =====
	renderStatusEffectsWindow();

	// ===== Crafting window (toggle with C) =====
	renderCraftingWindow();

	// ===== AoE Targeting overlay =====
	renderTargetingOverlay();

	// ===== Debug windows (only when debug is on) =====

	if (m_showDebug)
	{
		// Game mode
		ImGui::SetNextWindowPos(ImVec2(0, 370), ImGuiCond_FirstUseEver, ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(260, 110), ImGuiCond_FirstUseEver);
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
		ImGui::Text("Game Mode: %s", modeName(m_turnManager.GetGameMode()));
		ImGui::Text("Turn Queue: %d", m_turnManager.CurrentActor());
		ImGui::Text("Player Turn: %s", m_turnManager.IsPlayerTurn() ? "yes" : "no");
		ImGui::Text("Repeat Delay: %.3f s", m_moveRepeatDelay);
		ImGui::End();

		// Camera debug
		ImGui::SetNextWindowPos(ImVec2(0, 490), ImGuiCond_FirstUseEver, ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(260, 160), ImGuiCond_FirstUseEver);
		ImGui::Begin("Camera Debug");
		const glm::vec3 camPos = m_camera.Position();
		const glm::vec3 fwd = m_camera.Forward();
		ImGui::Text("Pos: %.2f, %.2f, %.2f", camPos.x, camPos.y, camPos.z);
		ImGui::Text("Forward: %.2f, %.2f, %.2f", fwd.x, fwd.y, fwd.z);
		ImGui::Text("Grid: [%d, %d, %d]",
			m_camera.GetGridPosition().row,
			m_camera.GetGridPosition().col,
			m_camera.GetGridPosition().floor);
		ImGui::Text("Facing: %s",
			m_camera.Facing() == Direction::North ? "North" :
			m_camera.Facing() == Direction::East  ? "East"  :
			m_camera.Facing() == Direction::South ? "South" : "West");
		ImGui::Text("Animating: %s", m_camera.IsAnimating() ? "yes" : "no");
		if (m_renderer != nullptr)
		{
			ImGui::Text("Meshes: %d / %d", m_renderer->DrawnMeshes(), m_renderer->TotalMeshes());
		}
		ImGui::End();

		// Grid position info
		ImGui::SetNextWindowPos(ImVec2(0, 660), ImGuiCond_FirstUseEver, ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(260, 90), ImGuiCond_FirstUseEver);
		ImGui::Begin("Dungeon Info");
		const GridPosition gp = m_camera.GetGridPosition();
		const Cell& cell = m_dungeon.GetCell(gp);
		ImGui::Text("Wall: %s", cell.isWall ? "yes" : "no");
		ImGui::Text("Floor: %s", cell.hasFloor ? "yes" : "no");
		ImGui::Text("Walkable: %s", cell.IsWalkable() ? "yes" : "no");
		ImGui::End();
	}

	// ---- Level Up overlay ----
	if (m_showLevelUp)
	{
		ImGui::OpenPopup("Level Up!");
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("Level Up!", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f),
				"Level %d!", m_character.GetLevel() + 1);
			ImGui::Separator();
			ImGui::Text("Choose a stat to increase (+2):");
			ImGui::Dummy(ImVec2(0.0f, 8.0f));

			auto applyLevelUp = [&](const char* statName, int32_t /*statValue*/, int32_t newValue)
			{
				m_character.SetLevel(m_character.GetLevel() + 1);
				m_character.SetAtkBonus(m_character.GetAtkBonus() + 1);
				m_character.SetStr(statName[0] == 'S' ? newValue : m_character.GetStr());
				m_character.SetDex(statName[0] == 'D' ? newValue : m_character.GetDex());
				m_character.SetCon(statName[0] == 'C' ? newValue : m_character.GetCon());
				m_character.SetMaxHp(m_character.GetMaxHp() + ExperienceSystem::HpGainForLevel(m_character.GetClass()));
				ExperienceSystem::GrantSkillsForLevel(m_character, m_character.GetLevel());
				ExperienceSystem::ApplyPassiveSkills(m_character);
				m_character.SetHp(m_character.GetMaxHp());
				m_character.SetXpForNext(ExperienceSystem::XpForLevel(m_character.GetLevel() + 1));
				m_showLevelUp = false;
				m_combatLog.Add(std::string("Level up! ") + statName + " increased!", glm::vec3(0.2f, 1.0f, 0.2f));
				ImGui::CloseCurrentPopup();
			};

			if (ImGui::Button("STR"))
			{
				applyLevelUp("STR", m_character.GetStr(), m_character.GetStr() + 2);
			}
			ImGui::SameLine();
			if (ImGui::Button("DEX"))
			{
				applyLevelUp("DEX", m_character.GetDex(), m_character.GetDex() + 2);
			}
			ImGui::SameLine();
			if (ImGui::Button("CON"))
			{
				applyLevelUp("CON", m_character.GetCon(), m_character.GetCon() + 2);
			}
			ImGui::EndPopup();
		}
	}

	// ---- Game Over overlay ----
	if (m_turnManager.GetGameMode() == GameMode::GameOver)
	{
		ImGui::OpenPopup("Game Over");
		ImVec2 center = ImGui::GetMainViewport()->GetCenter();
		ImGui::SetNextWindowPos(center, ImGuiCond_Always, ImVec2(0.5f, 0.5f));
		if (ImGui::BeginPopupModal("Game Over", nullptr,
			ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove))
		{
			ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f),
				"Your hero has fallen!");
			ImGui::Separator();
			if (ImGui::Button("Restart"))
			{
				RestartGame();
				ImGui::CloseCurrentPopup();
			}
			ImGui::SameLine();
			if (ImGui::Button("Back to Menu"))
			{
				m_machine.ReplaceState("MainMenu");
			}
			ImGui::EndPopup();
		}
	}
}

//=============================================================================
void PlayState::renderHotbar() noexcept
{
	ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always, ImVec2(0, 0));
	ImGui::SetNextWindowBgAlpha(0.0f);
	ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0));

	int32_t windowW = static_cast<int32_t>(m_window.Width());
	ImGui::SetNextWindowSize(ImVec2(static_cast<float>(windowW), 0), ImGuiCond_Always);

	ImGui::Begin("Hotbar", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoSavedSettings);

	float slotW = 48.0f;
	float slotH = 48.0f;
	float gap = 4.0f;
	float totalW = Character::NUM_ACTION_SLOTS * (slotW + gap) - gap;
	ImGui::SetCursorPosX((static_cast<float>(windowW) - totalW) * 0.5f);
	ImGui::SetCursorPosY(static_cast<float>(m_window.Height()) - slotH - 8.0f);

	for (int32_t i = 0; i < Character::NUM_ACTION_SLOTS; ++i)
	{
		const auto& slot = m_character.GetActionSlots()[i];

		ImGui::BeginGroup();
		ImVec2 cursor = ImGui::GetCursorScreenPos();

		// Background
		ImU32 bgCol = IM_COL32(40, 40, 40, 200);
		if (!slot.id.empty())
		{
			bgCol = IM_COL32(50, 50, 80, 220);
		}
		ImGui::GetWindowDrawList()->AddRectFilled(cursor,
			ImVec2(cursor.x + slotW, cursor.y + slotH), bgCol, 4.0f);
		ImGui::GetWindowDrawList()->AddRect(cursor,
			ImVec2(cursor.x + slotW, cursor.y + slotH),
			IM_COL32(100, 100, 120, 255), 4.0f);

		// Key label
		char keyLabel[4];
		std::snprintf(keyLabel, sizeof(keyLabel), "%d", i + 1);
		ImGui::GetWindowDrawList()->AddText(
			ImVec2(cursor.x + 3.0f, cursor.y + 2.0f),
			IM_COL32(180, 180, 200, 255), keyLabel);

		// Skill name
		if (!slot.id.empty())
		{
			Skill s = SkillManager::GetSkill(slot.id);
			std::string label = s.name.substr(0, 6);

			// MP cost indicator
			if (s.mpCost > 0)
			{
				char mpText[8];
				std::snprintf(mpText, sizeof(mpText), "%d MP", s.mpCost);
				ImGui::GetWindowDrawList()->AddText(
					ImVec2(cursor.x + 3.0f, cursor.y + slotH - 14.0f),
					IM_COL32(100, 150, 255, 200), mpText);
			}

			bool insufficientMp = (s.mpCost > m_character.GetMp());
			ImU32 textColor = IM_COL32(200, 200, 220, 255);

			// Cooldown overlay
			if (slot.cooldownRemaining > 0)
			{
				ImGui::GetWindowDrawList()->AddRectFilled(
					cursor, ImVec2(cursor.x + slotW, cursor.y + slotH),
					IM_COL32(0, 0, 0, 160), 4.0f);
				char cdText[8];
				std::snprintf(cdText, sizeof(cdText), "%d", slot.cooldownRemaining);
				ImVec2 textSz = ImGui::CalcTextSize(cdText);
				ImGui::GetWindowDrawList()->AddText(
					ImVec2(cursor.x + (slotW - textSz.x) * 0.5f,
						cursor.y + (slotH - textSz.y) * 0.5f),
					IM_COL32(255, 100, 100, 255), cdText);
			}
			else if (insufficientMp)
			{
				// Dim blue overlay for insufficient MP
				ImGui::GetWindowDrawList()->AddRectFilled(
					cursor, ImVec2(cursor.x + slotW, cursor.y + slotH),
					IM_COL32(0, 0, 80, 100), 4.0f);
				textColor = IM_COL32(100, 100, 200, 200);
				ImVec2 textSz = ImGui::CalcTextSize(label.c_str());
				ImGui::GetWindowDrawList()->AddText(
					ImVec2(cursor.x + (slotW - textSz.x) * 0.5f,
						cursor.y + slotH - textSz.y - 4.0f),
					textColor, label.c_str());
			}
			else
			{
				ImVec2 textSz = ImGui::CalcTextSize(label.c_str());
				ImGui::GetWindowDrawList()->AddText(
					ImVec2(cursor.x + (slotW - textSz.x) * 0.5f,
						cursor.y + slotH - textSz.y - 4.0f),
					textColor, label.c_str());
			}
		}

		ImGui::Dummy(ImVec2(slotW, slotH));
		ImGui::EndGroup();

		if (i < Character::NUM_ACTION_SLOTS - 1)
		{
			ImGui::SameLine(0.0f, gap);
		}
	}

	ImGui::End();
	ImGui::PopStyleColor();
}

//=============================================================================
void PlayState::renderSkillsWindow() noexcept
{
	if (!m_showSkills)
	{
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
	ImGui::Begin("Skills (K)", &m_showSkills);

	std::vector<Skill> allSkills = SkillManager::GetSkillsForClass(m_character.GetClass());

	// Split into passive and active
	ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Active Skills");
	ImGui::Separator();

	for (const auto& s : allSkills)
	{
		if (s.passive)
		{
			continue;
		}

		bool unlocked = false;
		for (const auto& us : m_character.GetUnlockedSkills())
		{
			if (us == s.id)
			{
				unlocked = true;
				break;
			}
		}

		bool available = s.levelReq <= m_character.GetLevel();

		if (!available)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
		}
		else if (!unlocked)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		}

		ImGui::Text("%s (Lvl %d)", s.name.c_str(), s.levelReq);
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text("%s", s.description.c_str());
			ImGui::Text("MP: %d  Cooldown: %d", s.mpCost, s.cooldown);
			ImGui::Text("Range: %d  Type: %s", s.range, s.type.c_str());
			ImGui::EndTooltip();
		}

		if (available && unlocked)
		{
			ImGui::SameLine();
			if (ImGui::SmallButton(("Assign##" + s.id).c_str()))
			{
				auto& slots = m_character.GetActionSlots();
				for (auto& slot : slots)
				{
					if (slot.id.empty())
					{
						slot.type = "ability";
						slot.id = s.id;
						break;
					}
				}
			}
		}

		ImGui::PopStyleColor();
	}

	ImGui::Dummy(ImVec2(0.0f, 8.0f));
	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Passive Skills");
	ImGui::Separator();

	for (const auto& s : allSkills)
	{
		if (!s.passive)
		{
			continue;
		}

		bool owned = s.levelReq <= m_character.GetLevel();
		if (!owned)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 1.0f, 0.8f, 1.0f));
		}

		ImGui::Text("%s (Lvl %d)", s.name.c_str(), s.levelReq);
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text("%s", s.description.c_str());
			if (s.hpBonus > 0) ImGui::Text("+%d HP", s.hpBonus);
			if (s.mpBonus > 0) ImGui::Text("+%d MP", s.mpBonus);
			if (s.atkBonus > 0) ImGui::Text("+%d ATK", s.atkBonus);
			if (s.acBonus > 0) ImGui::Text("+%d AC", s.acBonus);
			if (s.damageBonus > 0) ImGui::Text("+%d Damage", s.damageBonus);
			ImGui::EndTooltip();
		}
		ImGui::PopStyleColor();
	}

	ImGui::End();
}

//=============================================================================
void PlayState::renderEquipmentWindow() noexcept
{
	if (!m_showEquipment)
	{
		return;
	}

	ImGui::SetNextWindowPos(ImVec2(
		ImGui::GetMainViewport()->WorkSize.x * 0.5f - 175.0f,
		ImGui::GetMainViewport()->WorkSize.y * 0.5f - 200.0f
	), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(350, 400), ImGuiCond_FirstUseEver);

	if (!ImGui::Begin("Equipment (E)", &m_showEquipment))
	{
		ImGui::End();
		return;
	}

	Equipment& eq = m_character.GetEquipment();

	struct SlotInfo final
	{
		const char* label;
		EquipmentSlot slot;
	};

	const SlotInfo slots[] =
	{
		{"Weapon",  EquipmentSlot::Weapon},
		{"Shield",  EquipmentSlot::Shield},
		{"Head",    EquipmentSlot::Head},
		{"Body",    EquipmentSlot::Body},
		{"Hands",   EquipmentSlot::Hands},
		{"Feet",    EquipmentSlot::Feet},
		{"Ring",    EquipmentSlot::Ring},
		{"Amulet",  EquipmentSlot::Amulet}
	};

	ItemStats totals = eq.GetTotalStats();

	for (const auto& si : slots)
	{
		const Item* item = eq.Get(si.slot);

		ImGui::PushID(si.label);

		ImGui::Text("%s: ", si.label);
		ImGui::SameLine();

		if (item)
		{
			ImGui::TextColored(ImVec4(0.3f, 0.8f, 1.0f, 1.0f), "%s", item->name.c_str());
			if (ImGui::IsItemHovered())
			{
				ImGui::BeginTooltip();
				ImGui::Text("Rarity: ");
				ImGui::SameLine();
				switch (item->rarity)
				{
					case ItemRarity::Common:    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 1.0f), "Common"); break;
					case ItemRarity::Uncommon:  ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.3f, 1.0f), "Uncommon"); break;
					case ItemRarity::Rare:      ImGui::TextColored(ImVec4(0.3f, 0.5f, 1.0f, 1.0f), "Rare"); break;
					case ItemRarity::Legendary: ImGui::TextColored(ImVec4(1.0f, 0.7f, 0.0f, 1.0f), "Legendary"); break;
				}
				if (item->damageMin > 0 || item->damageMax > 0)
				{
					ImGui::Text("Damage: %d-%d", item->damageMin, item->damageMax);
				}
				if (item->ac > 0)
				{
					ImGui::Text("AC: +%d", item->ac);
				}
				if (item->atkBonus > 0)
				{
					ImGui::Text("ATK: +%d", item->atkBonus);
				}
				if (item->strBonus > 0)  { ImGui::Text("STR: +%d", item->strBonus); }
				if (item->dexBonus > 0)  { ImGui::Text("DEX: +%d", item->dexBonus); }
				if (item->conBonus > 0)  { ImGui::Text("CON: +%d", item->conBonus); }
				if (item->hpBonus > 0)   { ImGui::Text("HP: +%d", item->hpBonus); }
				if (item->mpBonus > 0)   { ImGui::Text("MP: +%d", item->mpBonus); }
				if (!item->elementType.empty())
				{
					ImGui::Text("Element: %s (%d-%d)",
						item->elementType.c_str(),
						item->elementDamageMin,
						item->elementDamageMax);
				}
				if (item->lifeStealPercent > 0)
				{
					ImGui::Text("Life Steal: %d%%", item->lifeStealPercent);
				}
				ImGui::Text("Value: %d", item->value);
				ImGui::EndTooltip();
			}

			ImGui::SameLine();
			if (ImGui::SmallButton("Unequip"))
			{
				std::optional<Item> unequipped = eq.Unequip(si.slot);
				if (unequipped.has_value())
				{
					m_character.GetInventory().Add(std::move(unequipped.value()));
				}
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "(empty)");
		}

		ImGui::PopID();
	}

	ImGui::Separator();
	ImGui::Text("Total Equipment Stats:");
	if (totals.damageMin > 0 || totals.damageMax > 0)
	{
		ImGui::Text("  Damage: +%d-%d", totals.damageMin, totals.damageMax);
	}
	if (totals.ac > 0)      { ImGui::Text("  AC: +%d", totals.ac); }
	if (totals.atkBonus > 0){ ImGui::Text("  ATK: +%d", totals.atkBonus); }
	if (totals.strBonus > 0){ ImGui::Text("  STR: +%d", totals.strBonus); }
	if (totals.dexBonus > 0){ ImGui::Text("  DEX: +%d", totals.dexBonus); }
	if (totals.conBonus > 0){ ImGui::Text("  CON: +%d", totals.conBonus); }
	if (totals.hpBonus > 0) { ImGui::Text("  HP: +%d", totals.hpBonus); }
	if (totals.mpBonus > 0) { ImGui::Text("  MP: +%d", totals.mpBonus); }

	ImGui::End();
}

//=============================================================================
void PlayState::ProcessSpellAction() noexcept
{
	if (!m_targetingActive)
	{
		return;
	}

	Skill spell = SkillManager::GetSkill(m_targetingSpellId);
	if (spell.id.empty())
	{
		m_targetingActive = false;
		m_targetingSpellId.clear();
		return;
	}

	if (spell.mpCost > m_character.GetMp())
	{
		m_combatLog.Add("Not enough MP!", glm::vec3(0.8f, 0.4f, 0.8f));
		m_targetingActive = false;
		m_targetingSpellId.clear();
		return;
	}

	if (m_turnManager.GetGameMode() != GameMode::Exploring
		&& m_turnManager.GetGameMode() != GameMode::TurnWaiting)
	{
		m_targetingActive = false;
		m_targetingSpellId.clear();
		return;
	}

	int32_t spellRange = spell.range;
	if (spellRange < 1) { spellRange = 1; }

	SpellTarget target = m_spellSystem.AcquireTarget(spell, spellRange);

	if (spell.type != "self" && target.hitMonsters.empty() && !target.hitWall)
	{
		m_combatLog.Add("No target in range!", glm::vec3(0.8f, 0.4f, 0.2f));
		m_targetingActive = false;
		m_targetingSpellId.clear();
		return;
	}

	m_spellSystem.ApplySpell(spell, target);

	// If this came from a wand, decrement charges or remove the wand
	if (!m_wandItemSpellId.empty())
	{
		auto* inv = &m_character.GetInventory();
		for (size_t wi = 0; wi < inv->Size(); ++wi)
		{
			const Item* wandItem = inv->Get(wi);
			if (wandItem && wandItem->type == ItemType::Wand && wandItem->spellId == m_wandItemSpellId)
			{
				Item modified = *wandItem;
				modified.charges--;
				inv->Remove(wi);
				if (modified.charges > 0)
				{
					inv->Add(std::move(modified));
				}
				else
				{
					m_combatLog.Add("Wand is now out of charges and crumbles to dust!",
						glm::vec3(0.8f, 0.6f, 0.2f));
				}
				break;
			}
		}
		m_wandItemSpellId.clear();
	}

	m_targetingActive = false;
	m_targetingSpellId.clear();
	m_targetingModeActive = false;
	m_showTargetingPreview = false;
	m_targetPreview = SpellTarget{};
	m_debugRenderer.ClearOverlay();

	m_turnManager.SetGameMode(GameMode::TurnWaiting);
}

//=============================================================================
bool PlayState::processGather() noexcept
{
	if (m_turnManager.GetGameMode() != GameMode::Exploring)
	{
		return false;
	}

	// Check the cell in front of the player
	GridPosition playerPos = m_camera.GetGridPosition();
	Direction facing = m_camera.Facing();
	glm::ivec2 offset = DirectionToVec(facing);
	GridPosition frontPos = {playerPos.row + offset.x, playerPos.col + offset.y, playerPos.floor};

	if (!Chunk::InBounds(frontPos.row, frontPos.col))
	{
		return false;
	}

	Cell& cell = m_dungeon.GetCell(frontPos);
	if (!cell.isResourceNode)
	{
		return false;
	}

	Inventory& inv = m_character.GetInventory();

	// Determine resource type and tool requirement
	// For now: random resource from a pool based on node type
	// Future: resource type stored in cell
	std::string resourceId;
	int32_t xpGain = 0;

	// Simple random: pick a mining/herbalism/fishing resource
	// We'll use a basic deterministic approach based on position
	int32_t posHash = frontPos.row * 31 + frontPos.col * 17;
	int32_t roll = posHash % 10;

	// Check if player has appropriate tool equipped
	const Item* weapon = m_character.GetEquipment().Get(EquipmentSlot::Weapon);
	bool hasPickaxe = weapon && weapon->itemId == "pickaxe";
	bool hasFishingRod = weapon && weapon->itemId == "fishing_rod";

	if (roll < 4) // Mining resources — need pickaxe
	{
		if (!hasPickaxe)
		{
			m_combatLog.Add("Need a pickaxe to mine here!", glm::vec3(1.0f, 0.6f, 0.0f));
			return true; // consumed action but failed
		}

		int32_t oreRoll = posHash % 3;
		if (oreRoll == 0)      { resourceId = "iron_ore"; xpGain = 5; }
		else if (oreRoll == 1) { resourceId = "coal"; xpGain = 4; }
		else                   { resourceId = "silver_ore"; xpGain = 8; }
	}
	else if (roll < 7) // Herbalism — no tool needed
	{
		int32_t herbRoll = posHash % 4;
		if (herbRoll == 0)      { resourceId = "herb"; xpGain = 5; }
		else if (herbRoll == 1) { resourceId = "flower"; xpGain = 3; }
		else if (herbRoll == 2) { resourceId = "mushroom"; xpGain = 5; }
		else                    { resourceId = "poison_herb"; xpGain = 6; }
	}
	else if (roll < 9) // Fishing — need fishing rod
	{
		if (!hasFishingRod)
		{
			m_combatLog.Add("Need a fishing rod to fish here!", glm::vec3(1.0f, 0.6f, 0.0f));
			return true; // consumed action but failed
		}
		resourceId = "fish";
		xpGain = 7;
	}
	else // Rare gem find
	{
		if (!hasPickaxe)
		{
			m_combatLog.Add("Need a pickaxe to mine this vein!", glm::vec3(1.0f, 0.6f, 0.0f));
			return true;
		}
		resourceId = "gem";
		xpGain = 15;
	}

	Item gathered = ItemFactory::CreateBase(resourceId);
	inv.Add(gathered);

	// Consume node (one-time use for now)
	cell.isResourceNode = false;

	m_combatLog.Add("Gathered " + gathered.name + "!", glm::vec3(0.2f, 0.9f, 0.2f));
	m_turnManager.SetGameMode(GameMode::TurnWaiting);

	// Award XP based on category
	m_craftingSystem.AddCookingXp(xpGain); // Using cooking XP for gathering for now

	return true;
}

//=============================================================================
void PlayState::processSearch() noexcept
{
	if (m_turnManager.GetGameMode() != GameMode::Exploring)
	{
		return;
	}

	GridPosition pos = m_camera.GetGridPosition();
	Cell& cell = m_dungeon.GetCell(pos);

	if (cell.isSecretWall)
	{
		cell.isWall = false;
		cell.isSecretWall = false;
		cell.hasFloor = true;
		m_dungeonRenderer.SetNeedsRebuild(true);
		m_combatLog.Add("You found a secret passage!", glm::vec3(0.2f, 0.8f, 1.0f));
		m_turnManager.SetGameMode(GameMode::TurnWaiting);
	}
	else if (cell.isTrap && !cell.isDisarmed)
	{
		cell.isDisarmed = true;
		m_combatLog.Add("You disarm the trap!", glm::vec3(0.2f, 0.8f, 0.2f));
		m_turnManager.SetGameMode(GameMode::TurnWaiting);
	}
	else
	{
		m_combatLog.Add("You search... nothing here.", glm::vec3(0.6f));
		m_turnManager.SetGameMode(GameMode::TurnWaiting);
	}
}

//=============================================================================
void PlayState::renderHungerIndicator() noexcept
{
	if (m_character.GetHp() <= 0)
	{
		return;
	}

	int32_t hunger = m_character.GetHunger();
	int32_t maxHunger = Character::MAX_HUNGER;

	ImVec4 color;
	const char* label;

	if (hunger >= maxHunger)
	{
		color = ImVec4(0.2f, 1.0f, 0.2f, 1.0f);
		label = "Full";
	}
	else if (hunger >= Character::HUNGER_WARNING)
	{
		color = ImVec4(1.0f, 1.0f, 0.2f, 1.0f);
		label = "Hungry";
	}
	else if (hunger > 0)
	{
		color = ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
		label = "Starving";
	}
	else
	{
		color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
		label = "Collapsed";
	}

	ImGui::Text("Hunger: ");
	ImGui::SameLine();
	float frac = static_cast<float>(hunger) / static_cast<float>(maxHunger);
	ImGui::PushStyleColor(ImGuiCol_PlotHistogram, ImVec4(color.x, color.y, color.z, 1.0f));
	ImGui::ProgressBar(frac, ImVec2(-1.0f, 0.0f),
		(std::to_string(hunger) + " / " + std::to_string(maxHunger) + " " + label).c_str());
	ImGui::PopStyleColor();

	// Show starvation penalty when active
	if (hunger < Character::HUNGER_STARVING)
	{
		ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.0f, 1.0f),
			"Starving: -1 ATK, -1 AC");
	}
}

//=============================================================================
void PlayState::renderStatusEffectsWindow() noexcept
{
	if (!m_showStatusEffects)
	{
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(250, 200), ImGuiCond_FirstUseEver);
	ImGui::Begin("Status Effects (U)", &m_showStatusEffects);

	ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Active Effects");
	ImGui::Separator();

	const auto& playerEffects = m_character.GetActiveEffects();
	if (playerEffects.empty())
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(none)");
	}
	else
	{
		for (const auto& effect : playerEffects)
		{
			const StatusEffectDef* def = m_spellSystem.GetStatusSystem().GetEffectDef(effect.defId);
			const char* name = def ? def->name.c_str() : effect.defId.c_str();
			ImGui::Text("%s", name);
			if (effect.remainingTurns > 0)
			{
				ImGui::SameLine();
				ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
					"(%d turn%s)", effect.remainingTurns, effect.remainingTurns == 1 ? "" : "s");
			}
		}
	}

	// Food buffs (step 224)
	ImGui::Dummy(ImVec2(0.0f, 4.0f));
	ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Food Buffs");
	ImGui::Separator();

	const auto& playerBuffs = m_character.GetActiveBuffs();
	if (playerBuffs.empty())
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(none)");
	}
	else
	{
		for (const auto& buff : playerBuffs)
		{
			ImGui::Text("%s", buff.name.c_str());
			ImGui::SameLine();
			ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f),
				"(%d turn%s)", buff.remainingTurns, buff.remainingTurns == 1 ? "" : "s");
		}
	}

	ImGui::End();
}

//=============================================================================
void PlayState::renderTargetingOverlay() noexcept
{
	if (!m_targetingModeActive)
	{
		return;
	}

	ImGui::SetNextWindowPos(ImVec2(
		ImGui::GetMainViewport()->WorkSize.x * 0.5f - 150.0f,
		ImGui::GetMainViewport()->WorkSize.y * 0.5f - 50.0f
	));
	ImGui::Begin("Targeting", nullptr,
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_AlwaysAutoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings);

	ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Select target for AoE");

	switch (m_currentTargetingMode)
	{
		case TargetingMode::Beam:
			ImGui::Text("Beam - fires in a line.");
			break;
		case TargetingMode::AoE_3x3:
			ImGui::Text("3x3 area around target cell.");
			break;
		case TargetingMode::AoE_5x5:
			ImGui::Text("5x5 area around target cell.");
			break;
		case TargetingMode::Cross:
			ImGui::Text("Cross pattern from target cell.");
			break;
		case TargetingMode::Line:
			ImGui::Text("Line - row or column.");
			break;
		default:
			break;
	}
	ImGui::Text("Press Space to confirm, Esc to cancel.");

	ImGui::End();
}

//=============================================================================
void PlayState::updateTargetingPreview() noexcept
{
	if (!m_targetingActive && !m_targetingModeActive)
	{
		if (m_showTargetingPreview)
		{
			m_showTargetingPreview = false;
			m_targetPreview = SpellTarget{};
			m_debugRenderer.ClearOverlay();
		}
		return;
	}

	std::string spellId = m_targetingSpellId;
	if (spellId.empty())
	{
		m_showTargetingPreview = false;
		return;
	}

	Skill spell = SkillManager::GetSkill(spellId);
	if (spell.id.empty())
	{
		m_showTargetingPreview = false;
		return;
	}

	int32_t spellRange = spell.range;
	if (spellRange < 1) { spellRange = 1; }

	// Acquire target preview based on current facing
	m_targetPreview = m_spellSystem.AcquireTarget(spell, spellRange);
	m_showTargetingPreview = true;
}

//=============================================================================
void PlayState::renderAoeGridOverlay() noexcept
{
	if (!m_showTargetingPreview)
	{
		return;
	}

	const glm::vec3 GREEN(0.0f, 1.0f, 0.2f);
	const glm::vec3 YELLOW(1.0f, 0.9f, 0.1f);
	const float HALF = 0.45f;
	const float Y = 0.05f; // slightly above floor

	auto drawCellOutline = [&](GridPosition cellPos, const glm::vec3& color)
	{
		glm::vec3 center = Camera::GridToWorld(cellPos);
		center.y = Y;

		// Four edges of the cell rectangle
		glm::vec3 bl = center + glm::vec3(-HALF, 0.0f, -HALF);
		glm::vec3 br = center + glm::vec3( HALF, 0.0f, -HALF);
		glm::vec3 tl = center + glm::vec3(-HALF, 0.0f,  HALF);
		glm::vec3 tr = center + glm::vec3( HALF, 0.0f,  HALF);

		m_debugRenderer.DrawOverlayLine(bl, br, color);
		m_debugRenderer.DrawOverlayLine(br, tr, color);
		m_debugRenderer.DrawOverlayLine(tr, tl, color);
		m_debugRenderer.DrawOverlayLine(tl, bl, color);
	};

	// Draw the target cell first
	drawCellOutline(m_targetPreview.position, YELLOW);

	// Draw each affected monster cell
	for (Monster* mon : m_targetPreview.hitMonsters)
	{
		if (mon && mon->alive)
		{
			drawCellOutline(mon->position, GREEN);
		}
	}
}

//=============================================================================
void PlayState::renderCraftingWindow() noexcept
{
	if (!m_showCrafting)
	{
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(400, 500), ImGuiCond_FirstUseEver);
	ImGui::Begin("Crafting (C)", &m_showCrafting);

	// Category tabs — render content INSIDE each tab item (standard ImGui pattern)
	const auto& categories = m_craftingSystem.Categories();
	if (ImGui::BeginTabBar("CraftCategories"))
	{
		for (int ci = 0; ci < static_cast<int>(categories.size()); ++ci)
		{
			if (ImGui::BeginTabItem(categories[ci].name.c_str()))
			{
				const std::string& catId = categories[ci].id;

				ImGui::Separator();
				ImGui::Text("Crafting Level: %d", m_craftingSystem.m_craftingLevel);
				ImGui::Separator();

				std::vector<const CraftingRecipe*> recipes = m_craftingSystem.GetRecipesByCategory(catId);

				if (recipes.empty())
				{
					ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "No recipes in this category.");
				}
				else
				{
					ImGui::BeginChild("RecipeList", ImVec2(0, 200), true);
					for (const CraftingRecipe* recipe : recipes)
					{
						bool unlocked = m_craftingSystem.IsRecipeUnlocked(recipe->id);
						bool canCraft = unlocked && m_craftingSystem.CanCraft(*recipe, m_character.GetInventory());

						ImGui::PushID(recipe->id.c_str());

						ImVec4 nameColor = unlocked ? ImVec4(1.0f, 1.0f, 1.0f, 1.0f)
													: ImVec4(0.4f, 0.4f, 0.4f, 1.0f);
						ImGui::TextColored(nameColor, "%s", recipe->name.c_str());

						if (unlocked)
						{
							ImGui::SameLine();
							ImGui::TextDisabled("(lvl %d)", recipe->skillReq);

							std::string ingredientsStr;
							for (size_t ii = 0; ii < recipe->ingredients.size(); ++ii)
							{
								if (ii > 0) ingredientsStr += ", ";
								ingredientsStr += recipe->ingredients[ii].itemId + " x" + std::to_string(recipe->ingredients[ii].count);
							}
							ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.7f, 1.0f), "  %s", ingredientsStr.c_str());
							ImGui::SameLine();
							ImGui::TextDisabled("-> %s x%d", recipe->result.itemId.c_str(), recipe->result.count);

							if (canCraft)
							{
								if (ImGui::Button("Craft"))
								{
									if (m_craftingSystem.Craft(*recipe, m_character.GetInventory()))
									{
										m_combatLog.Add("Crafted: " + recipe->name, glm::vec3(0.2f, 1.0f, 0.2f));
									}
								}
							}
							else
							{
								ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "  Missing ingredients");
							}
						}
						else
						{
							ImGui::TextColored(ImVec4(0.4f, 0.4f, 0.4f, 1.0f), "  (locked)");
						}

						ImGui::PopID();
						ImGui::Separator();
					}
					ImGui::EndChild();
				}

				// Category-specific operation UI
				if (catId == "weaponsmith")      { renderWeaponsmithOperations(); }
				else if (catId == "armorsmith")  { renderArmorsmithOperations(); }
				else if (catId == "alchemy")     { renderAlchemyOperations(); }
				else if (catId == "cooking")     { renderCookingOperations(); }

				ImGui::EndTabItem();
			}
		}
		ImGui::EndTabBar();
	}

	ImGui::End();
}

//=============================================================================
void PlayState::renderSpellbookWindow() noexcept
{
	if (!m_showSpellbook)
	{
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(400, 450), ImGuiCond_FirstUseEver);
	ImGui::Begin("Spellbook (B)", &m_showSpellbook);

	ImGui::Text("MP: %d / %d", m_character.GetMp(), m_character.GetMaxMp());
	ImGui::Separator();

	// Class spells
	ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Class Spells");
	ImGui::Separator();

	std::vector<std::string> spellIds = DataManager::GetSpellsByClass(m_character.GetClass());
	if (spellIds.empty())
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(none)");
	}

	for (const auto& sid : spellIds)
	{
		const json& spellJson = JsonDataManager::Instance().GetSpellData(sid);
		std::string name = spellJson.value("name", sid);
		int32_t lvlReq = spellJson.value("levelReq", 1);
		int32_t mpCost = spellJson.value("mpCost", 0);
		std::string desc = spellJson.value("description", "");

		bool available = lvlReq <= m_character.GetLevel();
		bool canAfford = mpCost <= m_character.GetMp();

		if (!available)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
		}
		else if (!canAfford)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.8f, 1.0f));
		}
		else
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
		}

		ImGui::Text("%s (Lvl %d, %d MP)", name.c_str(), lvlReq, mpCost);
		if (ImGui::IsItemHovered())
		{
			ImGui::BeginTooltip();
			ImGui::Text("%s", desc.c_str());
			ImGui::Text("Level Required: %d", lvlReq);
			ImGui::Text("MP Cost: %d", mpCost);
			ImGui::EndTooltip();
		}
		ImGui::PopStyleColor();

		if (available)
		{
			ImGui::SameLine();
			if (ImGui::SmallButton("Cast"))
			{
				m_targetingActive = true;
				m_targetingSpellId = sid;
				m_combatLog.Add("Select target for " + name + " (Space to cast, Esc to cancel).",
					glm::vec3(0.6f, 0.8f, 1.0f));
			}
			ImGui::SameLine();
			std::string assignLabel = "##assign_" + sid;
			if (ImGui::SmallButton("Hotbar"))
			{
				// Find first empty hotbar slot
				auto& slots = m_character.GetActionSlots();
				for (auto& slot : slots)
				{
					if (slot.id.empty())
					{
						slot.type = "spell";
						slot.id = sid;
						break;
					}
				}
			}
		}
	}

	// Learned spells
	ImGui::Dummy(ImVec2(0.0f, 8.0f));
	ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Learned Spells");
	ImGui::Separator();

	const auto& learned = m_character.GetLearnedSpells();
	if (learned.empty())
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(none)");
	}
	else
	{
		for (const auto& sid : learned)
		{
			const json& spellJson = JsonDataManager::Instance().GetSpellData(sid);
			std::string name = spellJson.value("name", sid);
			int32_t mpCost = spellJson.value("mpCost", 0);

			bool isClassSpell = false;
			for (const auto& cs : spellIds) { if (cs == sid) { isClassSpell = true; break; } }

			int32_t effectiveCost = isClassSpell ? mpCost : mpCost * 2;
			bool canAfford = effectiveCost <= m_character.GetMp();

			if (!canAfford)
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.4f, 0.8f, 1.0f));
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.8f, 1.0f, 0.8f, 1.0f));
			}

			ImGui::Text("%s (%d MP)%s",
				name.c_str(),
				effectiveCost,
				isClassSpell ? "" : " [Cross-class, x2 MP]");
			ImGui::PopStyleColor();

			ImGui::SameLine();
			if (ImGui::SmallButton(("Cast##" + sid).c_str()))
			{
				m_targetingActive = true;
				m_targetingSpellId = sid;
				m_combatLog.Add("Select target for " + name + " (Space to cast).",
					glm::vec3(0.6f, 0.8f, 1.0f));
			}
			ImGui::SameLine();
			if (ImGui::SmallButton(("Hot##" + sid).c_str()))
			{
				auto& slots = m_character.GetActionSlots();
				for (auto& slot : slots)
				{
					if (slot.id.empty())
					{
						slot.type = "spell";
						slot.id = sid;
						break;
					}
				}
			}
		}
	}

	ImGui::End();
}

//=============================================================================

void PlayState::renderWeaponsmithOperations() noexcept
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "--- Operations ---");

	Inventory& inv = m_character.GetInventory();

	// Step 174: Create weapon from baseType + material
	ImGui::Text("Create Weapon:");
	static int createBaseTypeIdx = 0;
	static int createMaterialIdx = 0;

	const char* baseTypes[] = {"Dagger", "Sword", "Axe", "Mace", "Spear", "Staff", "Bow", "Crossbow"};
	const char* baseTypeIds[] = {"dagger", "sword", "axe", "mace", "spear", "staff", "bow", "crossbow"};
	const char* materials[] = {"Iron", "Steel", "Silver", "Mithril", "Adamant"};
	const char* materialIds[] = {"iron", "steel", "silver", "mithril", "adamant"};

	ImGui::Combo("Base Type", &createBaseTypeIdx, baseTypes, IM_ARRAYSIZE(baseTypes));
	ImGui::Combo("Material", &createMaterialIdx, materials, IM_ARRAYSIZE(materials));

	if (ImGui::Button("Create Weapon"))
	{
		if (m_craftingSystem.HasIngredients(inv, "iron_ingot", 1))
		{
			if (m_craftingSystem.CreateWeapon(inv, baseTypeIds[createBaseTypeIdx], materialIds[createMaterialIdx]))
			{
				m_combatLog.Add("Created weapon!", glm::vec3(0.2f, 1.0f, 0.2f));
			}
		}
		else
		{
			m_combatLog.Add("Need 1 Iron Ingot to forge!", glm::vec3(1.0f, 0.3f, 0.0f));
		}
	}
	ImGui::Separator();

	// Step 175-177, 179: Operations on equipped weapon
	ImGui::Text("Current Weapon:");
	const Item* weapon = m_character.GetEquipment().Get(EquipmentSlot::Weapon);
	if (weapon && CraftingSystem::IsWeapon(*weapon))
	{
		ImGui::Text("  %s", weapon->name.c_str());
		ImGui::Text("  DMG: %d-%d  |  Durability: %d/%d  |  Sharpness: +%d",
			weapon->damageMin, weapon->damageMax,
			weapon->durability, weapon->maxDurability,
			weapon->sharpnessLevel);

		// Durability bar
		float durPct = weapon->maxDurability > 0
			? static_cast<float>(weapon->durability) / weapon->maxDurability
			: 0.0f;
		ImGui::ProgressBar(durPct, ImVec2(-1, 0), "");

		// Step 175: Upgrade weapon (prefix)
		if (ImGui::Button("Upgrade (use Iron Ingot)"))
		{
			if (m_craftingSystem.HasIngredients(inv, "iron_ingot", 1))
			{
				Item* w = m_character.GetEquipment().Get(EquipmentSlot::Weapon);
				if (w && m_craftingSystem.UpgradeWeapon(*w, "steel"))
				{
					m_combatLog.Add("Weapon upgraded!", glm::vec3(0.2f, 1.0f, 0.2f));
				}
				else
				{
					m_combatLog.Add("Upgrade failed (already at tier).", glm::vec3(1.0f, 0.3f, 0.0f));
				}
			}
			else
			{
				m_combatLog.Add("Need 1 Iron Ingot!", glm::vec3(1.0f, 0.3f, 0.0f));
			}
		}
		ImGui::SameLine();

		// Step 176: Sharpen
		if (ImGui::Button("Sharpen (use Whetstone)"))
		{
			if (m_craftingSystem.HasIngredients(inv, "whetstone", 1))
			{
				Item* w = m_character.GetEquipment().Get(EquipmentSlot::Weapon);
				if (w && m_craftingSystem.SharpenWeapon(*w))
				{
					m_combatLog.Add("Weapon sharpened!", glm::vec3(0.2f, 1.0f, 0.2f));
				}
				else
				{
					m_combatLog.Add("Sharpening failed (max +3).", glm::vec3(1.0f, 0.3f, 0.0f));
				}
			}
			else
			{
				m_combatLog.Add("Need 1 Whetstone!", glm::vec3(1.0f, 0.3f, 0.0f));
			}
		}
		ImGui::SameLine();

		// Step 179: Repair
		if (ImGui::Button("Repair"))
		{
			Item* w = m_character.GetEquipment().Get(EquipmentSlot::Weapon);
			if (w && m_craftingSystem.RepairItem(*w))
			{
				m_combatLog.Add("Weapon repaired!", glm::vec3(0.2f, 1.0f, 0.2f));
			}
			else
			{
				m_combatLog.Add("Repair failed (already full).", glm::vec3(1.0f, 0.3f, 0.0f));
			}
		}

		// Step 177: Add postfix
		ImGui::Text("Add Postfix:");
		static int postfixEssenceIdx = 0;
		const char* essences[] = {"Fire Essence", "Ice Essence", "Poison Essence"};
		const char* essenceIds[] = {"fire_essence", "ice_essence", "poison_essence"};
		ImGui::Combo("Essence", &postfixEssenceIdx, essences, IM_ARRAYSIZE(essences));
		ImGui::SameLine();
		if (ImGui::Button("Apply"))
		{
			if (m_craftingSystem.HasIngredients(inv, essenceIds[postfixEssenceIdx], 1))
			{
				Item* w = m_character.GetEquipment().Get(EquipmentSlot::Weapon);
				if (w && m_craftingSystem.AddPostfix(*w, essenceIds[postfixEssenceIdx]))
				{
					m_combatLog.Add("Postfix applied!", glm::vec3(0.2f, 1.0f, 0.2f));
				}
				else
				{
					m_combatLog.Add("Failed to apply postfix.", glm::vec3(1.0f, 0.3f, 0.0f));
				}
			}
			else
			{
				m_combatLog.Add("Need " + std::string(essences[postfixEssenceIdx]) + "!", glm::vec3(1.0f, 0.3f, 0.0f));
			}
		}
	}
	else
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "  No weapon equipped.");
	}

	// Step 182: Smithing level
	ImGui::Separator();
	ImGui::Text("Smithing XP: %d | Level: %d",
		m_craftingSystem.m_smithingXp,
		m_craftingSystem.GetSmithingLevel());
}

//=============================================================================

void PlayState::renderArmorsmithOperations() noexcept
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "--- Armorsmith Operations ---");

	Inventory& inv = m_character.GetInventory();

	// Step 183-184: Create armor from baseType + material
	ImGui::Text("Create Armor:");
	static int createArmorTypeIdx = 0;
	static int createArmorMatIdx = 0;

	const char* armorTypes[] = {"Helmet", "Armor", "Shield", "Boots", "Gloves"};
	const char* armorTypeIds[] = {"helmet", "plate", "shield", "boots", "gloves"};
	const char* materials[] = {"Iron", "Steel"};
	const char* materialIds[] = {"iron", "steel"};

	ImGui::Combo("Armor Type", &createArmorTypeIdx, armorTypes, IM_ARRAYSIZE(armorTypes));
	ImGui::Combo("Material", &createArmorMatIdx, materials, IM_ARRAYSIZE(materials));

	if (ImGui::Button("Create Armor"))
	{
		if (m_craftingSystem.HasIngredients(inv, "iron_ingot", 1))
		{
			if (m_craftingSystem.CreateArmor(inv, armorTypeIds[createArmorTypeIdx], materialIds[createArmorMatIdx]))
			{
				m_combatLog.Add("Created armor!", glm::vec3(0.2f, 1.0f, 0.2f));
			}
		}
		else
		{
			m_combatLog.Add("Need 1 Iron Ingot to forge!", glm::vec3(1.0f, 0.3f, 0.0f));
		}
	}
	ImGui::Separator();

	// Steps 185-187, 189: Operations on equipped armor pieces
	ImGui::Text("Equipped Armor:");
	EquipmentSlot armorSlots[] = {
		EquipmentSlot::Head, EquipmentSlot::Body, EquipmentSlot::Shield,
		EquipmentSlot::Hands, EquipmentSlot::Feet
	};
	const char* slotNames[] = {"Head", "Body", "Shield", "Hands", "Feet"};

	static int selectedArmorSlot = 0;
	ImGui::Combo("Slot", &selectedArmorSlot, slotNames, IM_ARRAYSIZE(slotNames));

	if (selectedArmorSlot >= 0 && selectedArmorSlot < 5)
	{
		EquipmentSlot slot = armorSlots[selectedArmorSlot];
		const Item* armor = m_character.GetEquipment().Get(slot);

		if (armor && CraftingSystem::IsArmor(*armor))
		{
			ImGui::Text("  %s", armor->name.c_str());
			ImGui::Text("  AC: %d  |  Durability: %d/%d",
				armor->ac, armor->durability, armor->maxDurability);

			float durPct = armor->maxDurability > 0
				? static_cast<float>(armor->durability) / armor->maxDurability
				: 0.0f;
			ImGui::ProgressBar(durPct, ImVec2(-1, 0), "");

			// Step 185: Upgrade
			if (ImGui::Button("Upgrade (use Iron Ingot)"))
			{
				Item* a = m_character.GetEquipment().Get(slot);
				if (a && m_craftingSystem.UpgradeArmor(*a, "steel"))
				{
					m_combatLog.Add("Armor upgraded!", glm::vec3(0.2f, 1.0f, 0.2f));
				}
				else
				{
					m_combatLog.Add("Upgrade failed.", glm::vec3(1.0f, 0.3f, 0.0f));
				}
			}
			ImGui::SameLine();

			// Step 187: Repair
			if (ImGui::Button("Repair"))
			{
				Item* a = m_character.GetEquipment().Get(slot);
				if (a && m_craftingSystem.RepairArmor(*a))
				{
					m_combatLog.Add("Armor repaired!", glm::vec3(0.2f, 1.0f, 0.2f));
				}
				else
				{
					m_combatLog.Add("Repair failed.", glm::vec3(1.0f, 0.3f, 0.0f));
				}
			}

			// Step 186: Postfix
			ImGui::Text("Add Postfix:");
			static int armorEssenceIdx = 0;
			const char* essences[] = {"Fire Essence", "Ice Essence", "Poison Essence", "Holy Essence"};
			const char* essenceIds[] = {"fire_essence", "ice_essence", "poison_essence", "holy_essence"};
			ImGui::Combo("Essence", &armorEssenceIdx, essences, IM_ARRAYSIZE(essences));
			ImGui::SameLine();
			if (ImGui::Button("Apply"))
			{
				Item* a = m_character.GetEquipment().Get(slot);
				if (a && m_craftingSystem.AddArmorPostfix(*a, essenceIds[armorEssenceIdx]))
				{
					m_combatLog.Add("Postfix applied!", glm::vec3(0.2f, 1.0f, 0.2f));
				}
				else
				{
					m_combatLog.Add("Failed.", glm::vec3(1.0f, 0.3f, 0.0f));
				}
			}

			// Step 189: Enchant (scroll + essence)
			ImGui::Text("Enchant:");
			static int enchantScrollIdx = 0;
			static int enchantEssenceIdx = 0;
			const char* scrolls[] = {"Fireball Scroll", "Frost Bolt Scroll", "Heal Scroll"};
			const char* scrollIds[] = {"scroll_fireball", "scroll_frost_bolt", "scroll_heal"};
			const char* enchantEssences[] = {"Fire Essence", "Ice Essence", "Holy Essence"};
			const char* enchantEssenceIds[] = {"fire_essence", "ice_essence", "holy_essence"};
			ImGui::Combo("Scroll", &enchantScrollIdx, scrolls, IM_ARRAYSIZE(scrolls));
			ImGui::Combo("Essence##enchant", &enchantEssenceIdx, enchantEssences, IM_ARRAYSIZE(enchantEssences));
			ImGui::SameLine();
			if (ImGui::Button("Enchant"))
			{
				if (m_craftingSystem.HasIngredients(inv, scrollIds[enchantScrollIdx], 1)
					&& m_craftingSystem.HasIngredients(inv, enchantEssenceIds[enchantEssenceIdx], 1))
				{
					Item* a = m_character.GetEquipment().Get(slot);
					if (a && m_craftingSystem.EnchantArmor(*a, scrollIds[enchantScrollIdx], enchantEssenceIds[enchantEssenceIdx]))
					{
						m_combatLog.Add("Armor enchanted!", glm::vec3(0.2f, 1.0f, 0.2f));
					}
					else
					{
						m_combatLog.Add("Enchant failed.", glm::vec3(1.0f, 0.3f, 0.0f));
					}
				}
				else
				{
					m_combatLog.Add("Need scroll + essence!", glm::vec3(1.0f, 0.3f, 0.0f));
				}
			}
		}
		else
		{
			ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "  No armor in this slot.");
		}
	}

	// Step 190: Armorsmith level
	ImGui::Separator();
	ImGui::Text("Armorsmith XP: %d | Level: %d",
		m_craftingSystem.m_armorsmithXp,
		m_craftingSystem.GetArmorsmithLevel());
}

//=============================================================================

void PlayState::renderAlchemyOperations() noexcept
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "--- Alchemy Operations ---");

	Inventory& inv = m_character.GetInventory();

	// Step 195: Apply poison to weapon
	ImGui::Text("Apply Poison to Weapon:");
	if (ImGui::Button("Apply Poison Vial") && m_craftingSystem.HasIngredients(inv, "poison_vial", 1))
	{
		Item* weapon = m_character.GetEquipment().Get(EquipmentSlot::Weapon);
		if (weapon && CraftingSystem::IsWeapon(*weapon))
		{
			weapon->elementType = "poison";
			weapon->elementDamageMin = 1;
			weapon->elementDamageMax = 4;
			m_craftingSystem.removeItems(inv, "poison_vial", 1);
			m_combatLog.Add("Poison applied to weapon! (+1d4 poison for 5 hits)", glm::vec3(0.2f, 1.0f, 0.0f));
			m_craftingSystem.AddAlchemyXp(10);
		}
		else
		{
			m_combatLog.Add("No weapon equipped!", glm::vec3(1.0f, 0.3f, 0.0f));
		}
	}

	// Step 196: Apply oil to weapon
	ImGui::Text("Apply Oil to Weapon:");
	static int oilTypeIdx = 0;
	const char* oils[] = {"Fire Oil", "Ice Oil"};
	const char* oilIds[] = {"oil_fire", "oil_ice"};
	ImGui::Combo("Oil Type", &oilTypeIdx, oils, IM_ARRAYSIZE(oils));
	ImGui::SameLine();
	if (ImGui::Button("Apply Oil"))
	{
		if (m_craftingSystem.HasIngredients(inv, oilIds[oilTypeIdx], 1))
		{
			Item* weapon = m_character.GetEquipment().Get(EquipmentSlot::Weapon);
			if (weapon && CraftingSystem::IsWeapon(*weapon))
			{
				std::string elem = (oilTypeIdx == 0) ? "fire" : "ice";
				weapon->elementType = elem;
				weapon->elementDamageMin = 2;
				weapon->elementDamageMax = 5;
				m_craftingSystem.removeItems(inv, oilIds[oilTypeIdx], 1);
				m_combatLog.Add("Weapon oiled with " + std::string(oils[oilTypeIdx]) + "!", glm::vec3(0.2f, 1.0f, 0.0f));
				m_craftingSystem.AddAlchemyXp(12);
			}
			else
			{
				m_combatLog.Add("No weapon equipped!", glm::vec3(1.0f, 0.3f, 0.0f));
			}
		}
		else
		{
			m_combatLog.Add("Missing oil!", glm::vec3(1.0f, 0.3f, 0.0f));
		}
	}

	// Step 198: Alchemy level
	ImGui::Separator();
	ImGui::Text("Alchemy XP: %d | Level: %d",
		m_craftingSystem.m_alchemyXp,
		m_craftingSystem.GetAlchemyLevel());
}

//=============================================================================

void PlayState::renderCookingOperations() noexcept
{
	ImGui::Separator();
	ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "--- Cooking Operations ---");

	ImGui::Text("Cooking Level affects dish quality and buff duration.");

	// Step 204: Cooking level
	ImGui::Separator();
	ImGui::Text("Cooking XP: %d | Level: %d",
		m_craftingSystem.m_cookingXp,
		m_craftingSystem.GetCookingLevel());
}

//=============================================================================
