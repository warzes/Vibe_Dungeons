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

		// ---- Character ----
		m_character = Character{};
		m_character.name = "Hero";
		m_character.hp = 20;
		m_character.maxHp = 20;
		m_character.ac = 12;
		m_character.atkBonus = 2;
		m_character.damageMin = 1;
		m_character.damageMax = 6;
		m_character.position = {17, 17, 0};
		m_character.facing = Direction::North;
		Logger::Info("PlayState: character created");

		// ---- Monster textures ----
		m_texMonsterSkeleton = m_resources.LoadTexture("tex_monster_skeleton", "data/skeleton.png", false);
		m_texMonsterSlime = m_resources.LoadTexture("tex_monster_slime", "data/slime.png", false);

		if (!m_texMonsterSkeleton || !m_texMonsterSlime)
		{
			throw std::runtime_error("Failed to load monster textures");
		}

		m_matMonsterSkeleton = m_resources.GetOrCreateMaterial("mat_monster_skeleton", *m_dungeonShader, *m_texMonsterSkeleton);
		m_matMonsterSlime = m_resources.GetOrCreateMaterial("mat_monster_slime", *m_dungeonShader, *m_texMonsterSlime);

		if (!m_matMonsterSkeleton || !m_matMonsterSlime)
		{
			throw std::runtime_error("Failed to create monster materials");
		}
		Logger::Info("PlayState: monster textures loaded");

		// ---- Monster renderer ----
		m_monsterRenderer.Init();

		// ---- Spawn test monsters ----
		{
			Monster skelly;
			skelly.name = "Skeleton";
			skelly.type = MonsterType::Skeleton;
			skelly.hp = 8;
			skelly.maxHp = 8;
			skelly.ac = 12;
			skelly.atkBonus = 1;
			skelly.damageMin = 1;
			skelly.damageMax = 4;
			skelly.position = {16, 17, 0};
			skelly.facing = Direction::South;
			m_monsterManager.Spawn(std::move(skelly));
		}
		{
			Monster slime;
			slime.name = "Slime";
			slime.type = MonsterType::Slime;
			slime.hp = 4;
			slime.maxHp = 4;
			slime.ac = 8;
			slime.atkBonus = 0;
			slime.damageMin = 1;
			slime.damageMax = 3;
			slime.position = {13, 17, 0};
			slime.facing = Direction::South;
			m_monsterManager.Spawn(std::move(slime));
		}
		Logger::Info("PlayState: monsters spawned");

		// ---- Item textures ----
		auto loadOrMakeTex = [&](const char* key, const char* file,
			int r1, int g1, int b1, int r2, int g2, int b2) -> Texture*
		{
			Texture* tex = m_resources.LoadTexture(key, file, false);
			if (!tex)
			{
				tex = m_resources.CreateCheckerboard(
					(std::string(key) + "_cb").c_str(), 32, 4,
					r1, g1, b1, 255, r2, g2, b2, 255);
			}
			return tex;
		};

		m_texItemHeal   = loadOrMakeTex("tex_item_heal",   "data/item_potion_heal.png", 255, 60, 60, 180, 0, 0);
		m_texItemMana   = loadOrMakeTex("tex_item_mana",   "data/item_potion_mana.png", 60, 120, 255, 0, 40, 180);
		m_texItemKey    = loadOrMakeTex("tex_item_key",    "data/item_key.png",          255, 215, 0, 180, 140, 0);
		m_texItemGold   = loadOrMakeTex("tex_item_gold",   "data/item_gold.png",         255, 200, 50, 200, 140, 0);
		m_texItemScroll = loadOrMakeTex("tex_item_scroll", "data/item_scroll.png",       240, 230, 200, 200, 180, 140);
		m_texItemSword  = loadOrMakeTex("tex_item_sword",  "data/item_sword.png",        180, 180, 200, 100, 100, 130);
		m_texItemArmor  = loadOrMakeTex("tex_item_armor",  "data/item_armor.png",        120, 120, 140, 70, 70, 90);
		m_texItemShield = loadOrMakeTex("tex_item_shield", "data/item_shield.png",       160, 120, 80, 110, 80, 50);
		m_texItemQuest  = loadOrMakeTex("tex_item_quest",  "data/item_quest.png",        180, 60, 200, 120, 20, 140);

		if (!m_texItemHeal || !m_texItemKey || !m_texItemGold)
		{
			throw std::runtime_error("Failed to create item textures");
		}

		auto makeMat = [&](const char* key, Texture& tex) -> Material*
		{
			return m_resources.GetOrCreateMaterial(key, *m_dungeonShader, tex);
		};

		auto* matHeal   = makeMat("mat_item_heal",   *m_texItemHeal);
		auto* matMana   = makeMat("mat_item_mana",   *m_texItemMana);
		auto* matKey    = makeMat("mat_item_key",    *m_texItemKey);
		auto* matGold   = makeMat("mat_item_gold",   *m_texItemGold);
		auto* matScroll = makeMat("mat_item_scroll", *m_texItemScroll);
		auto* matSword  = makeMat("mat_item_sword",  *m_texItemSword);
		auto* matArmor  = makeMat("mat_item_armor",  *m_texItemArmor);
		auto* matShield = makeMat("mat_item_shield", *m_texItemShield);
		auto* matQuest  = makeMat("mat_item_quest",  *m_texItemQuest);

		if (!matHeal || !matKey || !matGold)
		{
			throw std::runtime_error("Failed to create item materials");
		}
		Logger::Info("PlayState: item textures loaded");

		// ---- Item renderer ----
		m_itemRenderer.Init();
		m_itemRenderer.SetMaterial(ItemType::PotionHeal,  *matHeal);
		m_itemRenderer.SetMaterial(ItemType::PotionMana,  *matMana);
		m_itemRenderer.SetMaterial(ItemType::Key,          *matKey);
		m_itemRenderer.SetMaterial(ItemType::Gold,         *matGold);
		m_itemRenderer.SetMaterial(ItemType::Scroll,       *matScroll);
		m_itemRenderer.SetMaterial(ItemType::Weapon,       *matSword);
		m_itemRenderer.SetMaterial(ItemType::Armor,        *matArmor);
		m_itemRenderer.SetMaterial(ItemType::Shield,       *matShield);
		m_itemRenderer.SetMaterial(ItemType::QuestItem,    *matQuest);

		m_showInventory = false;

		// ---- Spawn test item drops ----
		{
			ItemDrop healPotion;
			healPotion.item.name = "Healing Potion";
			healPotion.item.type = ItemType::PotionHeal;
			healPotion.item.value = 10;
			healPotion.position = {16, 17, 0};
			m_itemDrops.push_back(std::move(healPotion));
		}
		{
			ItemDrop gold;
			gold.item.name = "Gold Coins";
			gold.item.type = ItemType::Gold;
			gold.item.value = 25;
			gold.position = {14, 17, 0};
			m_itemDrops.push_back(std::move(gold));
		}
		Logger::Info("PlayState: item drops spawned");

		// ---- Input actions for grid movement ----
		m_input.BindAction("GridMoveForward",  SDL_SCANCODE_W);
		m_input.BindAction("GridMoveBackward", SDL_SCANCODE_S);
		m_input.BindAction("TurnLeft",         SDL_SCANCODE_A);
		m_input.BindAction("TurnRight",        SDL_SCANCODE_D);
		m_input.BindAction("StrafeLeft",       SDL_SCANCODE_Q);
		m_input.BindAction("StrafeRight",      SDL_SCANCODE_E);
		m_input.BindAction("Action_Attack",    SDL_SCANCODE_SPACE);

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

	if (event.type == SDL_EVENT_KEY_DOWN && event.key.key == SDLK_I)
	{
		m_showInventory = !m_showInventory;
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

	if (m_gameMode == GameMode::GameOver)
	{
		// No input processing during GameOver
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

void PlayState::processAttack() noexcept
{
	if (m_gameMode != GameMode::Exploring && m_gameMode != GameMode::TurnAnimating)
	{
		return;
	}
	if (!m_input.IsActionPressed("Action_Attack"))
	{
		return;
	}

	// Context-sensitive: monster in front → attack; item on tile → pickup
	Monster* target = m_monsterManager.FindInFront(
		m_camera.GetGridPosition(),
		m_camera.Facing()
	);

	if (target)
	{
		performCombat();
	}
	else
	{
		processPickup();
	}
}

//=============================================================================

void PlayState::performCombat() noexcept
{
	Monster* target = m_monsterManager.FindInFront(
		m_camera.GetGridPosition(),
		m_camera.Facing()
	);

	if (!target)
	{
		m_combatLog.Add("Nothing to attack.", glm::vec3(0.6f));
		return;
	}

	// Player attacks
	AttackResult playerResult = m_combatSystem.MeleeAttack(m_character, *target);

	if (playerResult.critical)
	{
		m_combatLog.Add("Critical hit!", glm::vec3(1.0f, 0.2f, 0.2f));
	}

	if (playerResult.hit)
	{
		std::string msg = "Hero hits " + target->name + " for " +
			std::to_string(playerResult.damage) + " damage!";
		m_combatLog.Add(msg, glm::vec3(1.0f));
	}
	else
	{
		std::string msg = "Hero missed " + target->name + "!";
		m_combatLog.Add(msg, glm::vec3(0.7f, 0.7f, 0.7f));
	}

	if (playerResult.killed)
	{
		std::string msg = target->name + " dies!";
		m_combatLog.Add(msg, glm::vec3(0.2f, 1.0f, 0.2f));
		m_monsterManager.RemoveDead();
		m_gameMode = GameMode::TurnWaiting;
		return;
	}

	// Monster counter-attacks
	AttackResult monsterResult = m_combatSystem.MonsterMeleeAttack(*target, m_character);

	if (monsterResult.critical)
	{
		m_combatLog.Add("Critical hit!", glm::vec3(1.0f, 0.2f, 0.2f));
	}

	if (monsterResult.hit)
	{
		std::string msg = target->name + " hits Hero for " +
			std::to_string(monsterResult.damage) + " damage!";
		m_combatLog.Add(msg, glm::vec3(1.0f, 0.8f, 0.2f));
	}
	else
	{
		std::string msg = target->name + " missed Hero!";
		m_combatLog.Add(msg, glm::vec3(0.7f, 0.7f, 0.7f));
	}

	if (m_character.hp <= 0)
	{
		m_combatLog.Add("Hero has fallen!", glm::vec3(1.0f, 0.0f, 0.0f));
		m_gameMode = GameMode::GameOver;
		return;
	}

	m_gameMode = GameMode::TurnWaiting;
}

//=============================================================================

void PlayState::processPickup() noexcept
{
	GridPosition heroPos = m_camera.GetGridPosition();

	glm::ivec2 fwdDelta = DirectionToVec(m_camera.Facing());
	GridPosition frontPos(
		heroPos.row + fwdDelta.x,
		heroPos.col + fwdDelta.y,
		heroPos.floor
	);

	// Check the player's own cell first, then the cell in front
	GridPosition checkPositions[] = {heroPos, frontPos};

	for (const GridPosition& checkPos : checkPositions)
	{
		for (auto it = m_itemDrops.begin(); it != m_itemDrops.end(); ++it)
		{
			if (it->position.row == checkPos.row
				&& it->position.col == checkPos.col
				&& it->position.floor == checkPos.floor)
			{
				if (m_character.inventory.Add(it->item))
				{
					std::string msg = "Picked up " + it->item.name + "!";
					m_combatLog.Add(msg, glm::vec3(0.2f, 0.8f, 1.0f));
					m_itemDrops.erase(it);
					m_gameMode = GameMode::TurnWaiting;
					return;
				}
				else
				{
					m_combatLog.Add("Inventory full!", glm::vec3(1.0f, 0.5f, 0.0f));
					return;
				}
			}
		}
	}

	m_combatLog.Add("Nothing to attack or pick up.", glm::vec3(0.6f));
}

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

	ImGui::Text("Items: %zu / %zu", m_character.inventory.Size(), Inventory::MAX_ITEMS);
	ImGui::Separator();

	if (m_character.inventory.Size() == 0)
	{
		ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "(empty)");
	}
	else
	{
		for (size_t i = 0; i < m_character.inventory.Size(); ++i)
		{
			const Item* item = m_character.inventory.Get(i);
			if (!item)
			{
				continue;
			}

			ImGui::PushID(static_cast<int>(i));
			ImGui::Text("%s", item->name.c_str());

			if (item->type == ItemType::PotionHeal
				|| item->type == ItemType::PotionMana
				|| item->type == ItemType::Scroll)
			{
				ImGui::SameLine();
				if (ImGui::SmallButton("Use"))
				{
					if (item->type == ItemType::PotionHeal)
					{
						m_character.hp += item->value;
						if (m_character.hp > m_character.maxHp)
						{
							m_character.hp = m_character.maxHp;
						}
						m_combatLog.Add(
							"Used " + item->name + " (healed " + std::to_string(item->value) + " HP)!",
							glm::vec3(0.2f, 1.0f, 0.2f));
						m_character.inventory.Remove(i);
					}
					else if (item->type == ItemType::PotionMana)
					{
						m_combatLog.Add("Mana potions not yet implemented.",
							glm::vec3(0.6f));
					}
					else if (item->type == ItemType::Scroll)
					{
						m_combatLog.Add("Scrolls not yet implemented.",
							glm::vec3(0.6f));
					}
				}
			}

			ImGui::SameLine();
			if (ImGui::SmallButton("Drop"))
			{
				ItemDrop drop;
				drop.item = *item;
				drop.position = m_camera.GetGridPosition();
				m_itemDrops.push_back(std::move(drop));
				m_combatLog.Add("Dropped " + item->name + ".", glm::vec3(0.6f));
				m_character.inventory.Remove(i);
			}
			ImGui::PopID();
		}
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

	m_monsterRenderer.Submit(renderer, m_camera, m_monsterManager.All(),
		*m_matMonsterSkeleton, *m_matMonsterSlime);

	m_itemRenderer.Submit(renderer, m_camera, m_itemDrops);

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
	ImGui::Text("Space\tAttack");
	ImGui::Separator();
	ImGui::Text("Tab: Debug [%s]", m_showDebug ? "ON" : "OFF");
	if (ImGui::Button("Back to Menu"))
	{
		m_machine.ReplaceState("MainMenu");
	}
	ImGui::End();

	// ---- Hero HUD ----
	ImGui::SetNextWindowPos(ImVec2(0, 150), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(260, 110), ImGuiCond_FirstUseEver);
	ImGui::Begin("Hero");
	const float hpFrac = static_cast<float>(m_character.hp) / static_cast<float>(m_character.maxHp);
	ImGui::Text("%s", m_character.name.c_str());
	ImGui::ProgressBar(hpFrac, ImVec2(-1.0f, 0.0f),
		(std::to_string(m_character.hp) + " / " + std::to_string(m_character.maxHp)).c_str());
	ImGui::Text("AC: %d", m_character.ac);
	ImGui::Text("Attack Bonus: %+d", m_character.atkBonus);
	ImGui::Text("Damage: %dd%d", m_character.damageMin, m_character.damageMax);
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
		ImGui::Text("Game Mode: %s", modeName(m_gameMode));
		ImGui::Text("Turn Queue: %d", m_turnQueue.CurrentActor());
		ImGui::Text("Player Turn: %s", m_turnQueue.IsPlayerTurn() ? "yes" : "no");
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

	// ---- Game Over overlay ----
	if (m_gameMode == GameMode::GameOver)
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
			if (ImGui::Button("Back to Menu"))
			{
				m_machine.ReplaceState("MainMenu");
			}
			ImGui::EndPopup();
		}
	}
}
