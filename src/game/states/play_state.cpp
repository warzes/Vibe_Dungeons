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
#include "game/states/class_selection_state.h"

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
			m_pendingLevelUp
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

		m_itemHandler.SpawnDefault();
		Logger::Info("PlayState: item drops spawned");

		// ---- Spell system init ----
		m_spellSystem.Init(
			m_character,
			m_monsterManager,
			m_combatLog,
			m_dungeon
		);
		Logger::Info("PlayState: spell system initialized");

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
	m_turnManager.EndPlayerTurn(m_camera.IsAnimating());
}

//=============================================================================

void PlayState::processAttack() noexcept
{
	if (m_turnManager.GetGameMode() != GameMode::Exploring && m_turnManager.GetGameMode() != GameMode::TurnAnimating)
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
		m_combatHandler.PerformCombat();
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
				|| item->type == ItemType::Wand);

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
	ImGui::Text("Tab: Map [%s]", m_showMap ? "ON" : "OFF");
	ImGui::Text("F1: Debug [%s]", m_showDebug ? "ON" : "OFF");
	ImGui::Text("F2: Options");
	ImGui::Text("F5: Save / F9: Load");
	ImGui::Text("I: Inventory");
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

	m_turnManager.SetGameMode(GameMode::TurnWaiting);
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


