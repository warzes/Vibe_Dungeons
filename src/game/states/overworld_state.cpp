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
#include "game/data/npc_manager.h"
#include "game/data/quest_manager.h"
#include "game/data/shop_manager.h"
#include "game/data/item_factory.h"

#include <numbers>
//=============================================================================

namespace
{

const char* TERRAIN_NAMES[] =
{
	"Grassland", "Forest", "Mountain", "Water", "Road", "Desert"
};

const char* TERRAIN_KEY[] =
{
	"Grassland", "Forest", "Mountain", "Water", "Road", "Desert", nullptr
};

const char* terrainKey(TerrainType t) noexcept
{
	int32_t idx = static_cast<int32_t>(t);
	return (idx >= 0 && idx < 6) ? TERRAIN_KEY[idx] : nullptr;
}

} // anonymous namespace

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

	auto tex = std::make_unique<Texture>();
	tex->CreateFromRaw(W, H, reinterpret_cast<const uint32_t*>(pixels));
	return tex.release();
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

	// Night: double encounter chance (step 264)
	if (m_isNight)
	{
		chance = (std::min)(chance * 2.0f, 0.8f);
	}

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
// Day/night cycle (steps 263-265)
//=============================================================================

void OverworldState::advanceDayTime() noexcept
{
	m_dayTime = (m_dayTime + 1) % 24;
	if (m_dayTime == 0)
	{
		++m_dayCount;
	}
	updateAmbientLight();
}

void OverworldState::updateAmbientLight() noexcept
{
	// Day: 6..17 (bright), Dusk: 18-19 (fading), Night: 20-5 (dark), Dawn: 5 (rising)
	if (m_dayTime >= 6 && m_dayTime <= 17)
	{
		m_ambientLight = 0.6f;
		m_isNight = false;
	}
	else if (m_dayTime == 18) // Dusk
	{
		m_ambientLight = 0.45f;
		m_isNight = true;
	}
	else if (m_dayTime == 19)
	{
		m_ambientLight = 0.30f;
		m_isNight = true;
	}
	else if (m_dayTime >= 20 || m_dayTime == 0)
	{
		m_ambientLight = 0.20f;
		m_isNight = true;
	}
	else if (m_dayTime == 5) // Dawn
	{
		m_ambientLight = 0.40f;
		m_isNight = false;
	}
	else
	{
		m_ambientLight = 0.20f;
		m_isNight = true;
	}

	// Update view radius based on time (step 264)
	if (m_isNight)
	{
		m_viewRadius = 3;
	}
	else
	{
		m_viewRadius = 5;
	}
}

//=============================================================================

bool OverworldState::processOverworldTurn() noexcept
{
	// Hunger decreases each step (step 243)
	m_character.ConsumeHunger(1);
	m_character.TickBuffs();

	// Advance day/night (step 263)
	advanceDayTime();

	if (m_character.GetHunger() <= 0)
	{
		Logger::Warn("Overworld: starving! Taking damage.");
		m_character.TakeDamage(1);
	}

	if (m_character.GetHp() <= 0)
	{
		Logger::Info("Overworld: character died of starvation.");
		m_machine.ReplaceState("MainMenu");
		return false; // state was destroyed
	}

	return true;
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

		// ---- Load NPC / Quest / Shop data ----
		NpcManager::LoadNpcs("data/npcs.json");
		QuestManager::LoadQuests("data/quests.json");
		ShopManager::LoadShops("data/shop_tables.json");
		initNpcPositions();

		// ---- Day/night init ----
		m_dayTime = 6;
		updateAmbientLight();

		// ---- Location state ----
		m_showLocationPopup = false;
		m_activeLocationId.clear();
		m_shrineUsed = false;
		m_campRestUsed = false;

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

	// Restore character after returning from combat/dungeon (steps 246)
	if (m_pendingCharacter && m_pendingCharacter->GetLevel() > 0)
	{
		m_character = std::move(*m_pendingCharacter);
		*m_pendingCharacter = Character{};
		m_hasCharacter = true;

		// Sync camera with character position
		m_camera.SetGridPosition(m_character.GetPosition(), m_character.GetFacing());
		m_camera.SnapToGrid();
	}
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
			case SDLK_J:
				m_showQuestJournal = !m_showQuestJournal;
				m_showDialogue = false;
				m_showTrade = false;
				break;
			case SDLK_ESCAPE:
				if (m_showLocationPopup)
				{
					m_showLocationPopup = false;
				}
				else
				{
					m_machine.ReplaceState("MainMenu");
				}
				break;
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

	// Movement costs a turn: hunger, time, encounters, etc.
	if (wasMovement)
	{
		if (processOverworldTurn())
		{
			triggerRandomEncounter();
		}
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
// Location interaction (steps 251-256)
//=============================================================================

void OverworldState::processInteraction() noexcept
{
	if (!m_input.IsActionPressed("Action_Interact"))
	{
		return;
	}

	// If any popup is open, Space closes it
	if (m_showLocationPopup)
	{
		m_showLocationPopup = false;
		return;
	}
	if (m_showDialogue)
	{
		m_showDialogue = false;
		m_dialogueStep = 0;
		return;
	}
	if (m_showTrade)
	{
		m_showTrade = false;
		return;
	}
	if (m_showQuestJournal)
	{
		m_showQuestJournal = false;
		return;
	}

	GridPosition pos = m_camera.GetGridPosition();
	glm::ivec2 fwd = DirectionToVec(m_camera.Facing());
	GridPosition front = {pos.row + fwd.x, pos.col + fwd.y, 0};

	// Check for NPC at front cell first
	auto npcIt = m_npcPositions.find(front);
	if (npcIt != m_npcPositions.end())
	{
		processNpcInteraction();
		return;
	}

	const OverworldLocation* loc = m_overworld.FindLocationAt(pos);
	if (!loc)
	{
		loc = m_overworld.FindLocationAt(front);
	}

	if (loc)
	{
		Logger::Info(std::string("Interacting with location: ") + loc->name);
		m_overworld.MarkVisited(loc->position);
		processLocationInteraction(*loc);
	}
}

void OverworldState::processLocationInteraction(const OverworldLocation& loc) noexcept
{
	m_activeLocationId = loc.id;
	m_showLocationPopup = true;

	switch (loc.type)
	{
		case LocationType::Town:
			Logger::Info("Entering town: " + loc.name);
			break;
		case LocationType::DungeonEntrance:
			Logger::Info("Entering dungeon: " + loc.name);
			break;
		case LocationType::Shrine:
			Logger::Info("Approaching shrine: " + loc.name);
			break;
		case LocationType::Camp:
			Logger::Info("Approaching camp: " + loc.name);
			break;
	}
}

void OverworldState::processTownInteraction() noexcept
{
	// Rest: full heal, costs 10 gold
	int32_t restCost = 10;
	if (m_character.GetHp() < m_character.GetMaxHp() ||
		m_character.GetMp() < m_character.GetMaxMp())
	{
		// Check if player has gold
		bool hasGold = false;
		auto& inv = m_character.GetInventory();
		for (size_t i = 0; i < inv.Size(); ++i)
		{
			const Item* item = inv.Get(i);
			if (item && item->type == ItemType::Gold && item->value >= restCost)
			{
				hasGold = true;
				break;
			}
		}

		if (hasGold)
		{
			// Remove 10 gold
			int32_t toRemove = restCost;
			for (size_t i = 0; i < inv.Size() && toRemove > 0; )
			{
				Item* item = inv.Get(i);
				if (item && item->type == ItemType::Gold)
				{
					int32_t stackValue = item->value;
					if (stackValue <= toRemove)
					{
						toRemove -= stackValue;
						inv.Remove(i);
					}
					else
					{
						item->value = stackValue - toRemove;
						toRemove = 0;
						++i;
					}
				}
				else
				{
					++i;
				}
			}

			m_character.Heal(m_character.GetMaxHp());
			m_character.RestoreMp(m_character.GetMaxMp());
			Logger::Info("Town: rested, fully healed. Cost: 10 gold.");
		}
		else
		{
			Logger::Warn("Town: not enough gold to rest (need 10 gold)");
		}
	}
	else
	{
		Logger::Info("Town: already at full health.");
	}
}

void OverworldState::processShrineInteraction() noexcept
{
	if (m_shrineUsed)
	{
		Logger::Info("Shrine already used its power.");
		return;
	}

	// Full HP/MP restore + buff
	m_character.Heal(m_character.GetMaxHp());
	m_character.RestoreMp(m_character.GetMaxMp());

	// Apply Shrine Blessing buff
	Character::FoodBuff shrineBuff;
	shrineBuff.id = "shrine_blessing";
	shrineBuff.name = "Shrine Blessing";
	shrineBuff.atkBonus = 2;
	shrineBuff.acBonus = 2;
	shrineBuff.remainingTurns = 50;
	m_character.ApplyFoodBuff(shrineBuff);

	m_shrineUsed = true;
	Logger::Info("Shrine: restored HP/MP and applied Shrine Blessing (+2 ATK, +2 AC, 50 turns)");
}

void OverworldState::processCampInteraction() noexcept
{
	// Rest: heal 50% HP/MP, uses 1 food from inventory
	auto& inv = m_character.GetInventory();
	int32_t foodSlot = -1;
	for (size_t i = 0; i < inv.Size(); ++i)
	{
		const Item* item = inv.Get(i);
		if (item && item->type == ItemType::Food && item->expirationTurns != 0)
		{
			foodSlot = static_cast<int32_t>(i);
			break;
		}
	}

	if (foodSlot >= 0)
	{
		inv.Remove(foodSlot);
		m_character.Heal(m_character.GetMaxHp() / 2);
		m_character.RestoreMp(m_character.GetMaxMp() / 2);
		Logger::Info("Camp: rested, used 1 food. Restored 50% HP/MP.");
	}
	else
	{
		Logger::Warn("Camp: no fresh food to rest. Need food to rest at camp.");
	}
}

void OverworldState::processDungeonEntranceInteraction() noexcept
{
	// Push a combat encounter as dungeon entrance
	std::vector<EncounterSpawnEntry> spawnList;

	// Generate a simple group encounter appropriate for player level
	int32_t playerLevel = m_character.GetLevel();
	{
		EncounterSpawnEntry entry;
		entry.monsterTypeId = "skeleton";
		entry.count = 2;
		entry.level = playerLevel;
		spawnList.push_back(std::move(entry));
	}
	{
		EncounterSpawnEntry entry;
		entry.monsterTypeId = "slime";
		entry.count = 1;
		entry.level = playerLevel;
		spawnList.push_back(std::move(entry));
	}

	// Save character before transfer
	if (m_pendingCharacter)
	{
		*m_pendingCharacter = std::move(m_character);
	}

	CombatEncounterState::s_spawnEntries = std::move(spawnList);
	m_machine.PushState("CombatEncounter");
	m_showLocationPopup = false;
}

//=============================================================================

void OverworldState::initNpcPositions() noexcept
{
	m_npcPositions.clear();

	// Place NPCs at specific cells in Greyhaven town area (~row 56, col 10)
	// These positions should be walkable cells within the town bounds
	m_npcPositions[{55, 8, 0}]  = "merchant_alric";
	m_npcPositions[{55, 9, 0}]  = "weaponsmith_brun";
	m_npcPositions[{55, 11, 0}] = "alchemist_sera";
	m_npcPositions[{56, 8, 0}]  = "barkeeper_holt";
	m_npcPositions[{56, 9, 0}]  = "trainer_lyra";
	m_npcPositions[{56, 11, 0}] = "quest_giver_eldrin";

	Logger::Info(std::string("NPC positions initialized: ") + std::to_string(m_npcPositions.size()));
}

//=============================================================================

void OverworldState::processNpcInteraction() noexcept
{
	GridPosition pos = m_camera.GetGridPosition();
	glm::ivec2 fwd = DirectionToVec(m_camera.Facing());
	GridPosition front = {pos.row + fwd.x, pos.col + fwd.y, 0};

	auto npcIt = m_npcPositions.find(front);
	if (npcIt == m_npcPositions.end())
	{
		return;
	}

	const std::string& npcId = npcIt->second;
	m_activeNpcId = npcId;
	m_dialogueStep = 0;
	m_showDialogue = true;

	const Npc* npc = NpcManager::GetNpc(npcId);
	if (!npc)
	{
		Logger::Warn(std::string("NPC not found: ") + npcId);
		return;
	}

	Logger::Info(std::string("Talking to NPC: ") + npc->name);

	// Tick talk quests
	m_character.TickQuestTalk(npcId);

	// If NPC has shop table, refresh inventory on new day
	if (!npc->shopTableId.empty())
	{
		if (m_dayCount != m_lastShopRefreshDay)
		{
			m_shopInventories[npc->shopTableId] =
				ShopManager::GenerateInventory(npc->shopTableId, static_cast<int32_t>(m_rng()));
			m_lastShopRefreshDay = m_dayCount;
		}
	}

	// Check if NPC has available quests
	if (npc->type == NpcType::QuestGiver)
	{
		for (const auto& qId : npc->questIds)
		{
			if (!m_character.HasQuest(qId) && !m_character.IsQuestCompleted(qId))
			{
				const Quest* q = QuestManager::GetQuest(qId);
				if (q && m_character.GetLevel() >= q->levelReq)
				{
					Logger::Info(std::string("Quest available from NPC: ") + q->name);
				}
			}
		}
	}
}

//=============================================================================

void OverworldState::renderNpcDialogue() noexcept
{
	const Npc* npc = NpcManager::GetNpc(m_activeNpcId);
	if (!npc)
	{
		m_showDialogue = false;
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
	ImGui::Begin(std::string(npc->name + " - Dialogue").c_str(), &m_showDialogue);

	if (m_dialogueStep < static_cast<int32_t>(npc->dialogues.size()))
	{
		const auto& dia = npc->dialogues[m_dialogueStep];
		ImGui::TextWrapped("%s", dia.text.c_str());
		ImGui::Separator();

		for (size_t i = 0; i < dia.responses.size(); ++i)
		{
			if (ImGui::Button(dia.responses[i].c_str(), ImVec2(250, 0)))
			{
				m_dialogueStep++;
			}
		}

		// Show available quests if this is a quest giver
		if (npc->type == NpcType::QuestGiver && !npc->questIds.empty())
		{
			ImGui::Separator();
			ImGui::Text("--- Available Quests ---");
			for (const auto& qId : npc->questIds)
			{
				const Quest* q = QuestManager::GetQuest(qId);
				if (!q || m_character.HasQuest(qId) || m_character.IsQuestCompleted(qId))
				{
					continue;
				}
				if (m_character.GetLevel() < q->levelReq)
				{
					ImGui::TextDisabled("%s (Requires Lv %d)", q->name.c_str(), q->levelReq);
					continue;
				}
				ImGui::PushID(q->id.c_str());
				ImGui::TextWrapped("%s: %s", q->name.c_str(), q->description.c_str());
				if (ImGui::Button("Accept"))
				{
					m_character.AcceptQuest(qId);
				}
				ImGui::PopID();
			}
		}
	}
	else
	{
		ImGui::TextWrapped("(End of dialogue)");

		// Show Buy/Sell button for merchants
		if (npc->type == NpcType::Merchant && !npc->shopTableId.empty())
		{
			if (ImGui::Button("Open Shop", ImVec2(250, 0)))
			{
				m_showTrade = true;
				m_showDialogue = false;
			}
		}

		if (ImGui::Button("Goodbye", ImVec2(250, 0)))
		{
			m_showDialogue = false;
			m_dialogueStep = 0;
		}
	}

	ImGui::End();
}

//=============================================================================

void OverworldState::renderTradeWindow() noexcept
{
	const Npc* npc = NpcManager::GetNpc(m_activeNpcId);
	if (!npc)
	{
		m_showTrade = false;
		return;
	}

	ImGui::SetNextWindowSize(ImVec2(700, 400), ImGuiCond_FirstUseEver);
	ImGui::Begin(std::string(npc->name + " - Shop").c_str(), &m_showTrade);

	ImGui::Text("Your gold: %d", m_character.GetGold());

	// Determine reputation discount
	float buyMultiplier = 1.5f;
	float sellMultiplier = 0.3f;
	int32_t rep = m_character.GetReputation();
	if (rep >= 50)  { buyMultiplier = 1.2f; sellMultiplier = 0.5f; }
	if (rep >= 100) { buyMultiplier = 1.0f; sellMultiplier = 0.6f; }

	ImGui::Columns(2, "TradeColumns");
	ImGui::Separator();

	// ---- Shop inventory (buy) ----
	ImGui::Text("--- Buy ---");
	ImGui::BeginChild("BuyPanel", ImVec2(0, 300), true);

	// Generate or retrieve shop inventory
	auto shopInv = m_shopInventories.find(npc->shopTableId);
	if (shopInv == m_shopInventories.end())
	{
		// Generate on first access
		m_shopInventories[npc->shopTableId] =
			ShopManager::GenerateInventory(npc->shopTableId, static_cast<int32_t>(m_rng()));
		shopInv = m_shopInventories.find(npc->shopTableId);
	}

	if (shopInv != m_shopInventories.end())
	{
		for (size_t i = 0; i < shopInv->second.size(); ++i)
		{
			const std::string& itemId = shopInv->second[i];
			Item item = ItemFactory::CreateBase(itemId);
			int32_t price = static_cast<int32_t>(std::round(item.value * buyMultiplier));
			if (price < 1) price = 1;

			ImGui::PushID(static_cast<int32_t>(i));
			ImGui::Text("%s", item.name.c_str());
			ImGui::SameLine();
			ImGui::TextDisabled("%d gold", price);
			ImGui::SameLine();
			if (ImGui::SmallButton("Buy"))
			{
				if (m_character.GetGold() >= price)
				{
					if (m_character.GetInventory().Add(ItemFactory::CreateBase(itemId)) == AddResult::Success)
					{
						m_character.AddGold(-price);
						Logger::Info(std::string("Bought ") + itemId);
					}
				}
			}
			ImGui::PopID();
		}
	}

	ImGui::EndChild();

	ImGui::NextColumn();

	// ---- Player inventory (sell) ----
	ImGui::Text("--- Sell ---");
	ImGui::BeginChild("SellPanel", ImVec2(0, 300), true);

	auto& inv = m_character.GetInventory();
	size_t i = 0;
	while (i < inv.Size())
	{
		const Item* item = inv.Get(i);
		if (!item) { ++i; continue; }

		// Can't sell equipped items or quest items
		// (for simplicity, skip gold items too)
		if (item->type == ItemType::Gold) { ++i; continue; }
		if (item->type == ItemType::QuestItem) { ++i; continue; }

		int32_t sellPrice = static_cast<int32_t>(std::round(item->value * sellMultiplier));
		if (sellPrice < 1) sellPrice = 1;

		ImGui::PushID(static_cast<int32_t>(i));
		ImGui::Text("%s", item->name.c_str());
		ImGui::SameLine();
		ImGui::TextDisabled("%d gold", sellPrice);
		ImGui::SameLine();
		if (ImGui::SmallButton("Sell"))
		{
			m_character.AddGold(sellPrice);
			inv.Remove(i);
			Logger::Info("Sold item");
			ImGui::PopID();
			continue; // re-check same index after removal
		}
		ImGui::PopID();
		++i;
	}

	ImGui::EndChild();

	ImGui::Columns(1);
	ImGui::Separator();

	if (ImGui::Button("Close Shop", ImVec2(250, 0)))
	{
		m_showTrade = false;
	}

	ImGui::End();
}

//=============================================================================

void OverworldState::renderQuestJournal() noexcept
{
	ImGui::SetNextWindowSize(ImVec2(500, 400), ImGuiCond_FirstUseEver);
	ImGui::Begin("Quest Journal", &m_showQuestJournal);

	if (m_character.GetQuestLog().empty())
	{
		ImGui::TextWrapped("No quests accepted yet. Visit quest givers in towns to find work.");
		ImGui::End();
		return;
	}

	int32_t activeCount = 0;
	int32_t completedCount = 0;

	for (const auto& entry : m_character.GetQuestLog())
	{
		const Quest* q = QuestManager::GetQuest(entry.questId);
		if (!q) continue;

		if (entry.status == QuestStatus::Completed)
		{
			completedCount++;
		}
		else if (entry.status == QuestStatus::Active)
		{
			activeCount++;
		}

		const char* statusStr = entry.status == QuestStatus::Active ? "Active" :
			entry.status == QuestStatus::Completed ? "Completed" : "Inactive";

		ImGui::PushID(entry.questId.c_str());
		ImGui::TextColored(
			entry.status == QuestStatus::Completed ? ImVec4(0.5f, 0.5f, 0.5f, 1.0f) : ImVec4(1.0f, 1.0f, 1.0f, 1.0f),
			"%s [%s]", q->name.c_str(), statusStr);
		if (entry.status == QuestStatus::Active)
		{
			ImGui::TextWrapped("  %s", q->description.c_str());
			// Show objectives
			int32_t objIdx = 0;
			for (const auto& obj : q->objectives)
			{
				int32_t progress = objIdx < static_cast<int32_t>(entry.objectiveProgress.size())
					? entry.objectiveProgress[objIdx] : 0;
				const char* typeStr =
					obj.type == QuestObjectiveType::Kill ? "Kill" :
					obj.type == QuestObjectiveType::Collect ? "Collect" :
					obj.type == QuestObjectiveType::Explore ? "Explore" :
					obj.type == QuestObjectiveType::Talk ? "Talk" : "?";
				ImGui::Text("    %s %s: %d/%d", typeStr, obj.target.c_str(), progress, obj.amount);
				objIdx++;
			}
		}
		ImGui::PopID();
		ImGui::Separator();
	}

	ImGui::Text("Active: %d | Completed: %d", activeCount, completedCount);
	ImGui::Text("Reputation: %d", m_character.GetReputation());

	ImGui::End();
}

//=============================================================================

void OverworldState::RenderScene(Renderer& renderer) noexcept
{
	if (!m_initialized) return;

	m_renderer = &renderer;

	// Set ambient light uniform on the shader (step 263-264)
	m_overworldShader->Bind();
	m_overworldShader->SetUniform("uAmbientLight", m_ambientLight);

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
	ImGui::SetNextWindowSize(ImVec2(260, 200), ImGuiCond_FirstUseEver);
	ImGui::Begin("Overworld");
	ImGui::Text("World: %dx%d", OVERWORLD_SIZE, OVERWORLD_SIZE);
	ImGui::Separator();
	ImGui::Text("W/S \tMove Fwd/Back");
	ImGui::Text("A/D \tTurn L/R");
	ImGui::Text("Q/E \tStrafe L/R");
	ImGui::Text("Space\tInteract");
	ImGui::Text("T    \tFast Travel");
	ImGui::Separator();
	ImGui::Text("M: Map  J: Journal  Esc: Menu");

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

	// Day/night indicator in control panel (step 265)
	renderDayNightIndicator();

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

	// Show NPC ahead
	{
		glm::ivec2 fwd = DirectionToVec(m_camera.Facing());
		GridPosition front2 = {gp.row + fwd.x, gp.col + fwd.y, 0};
		auto npcIt = m_npcPositions.find(front2);
		if (npcIt != m_npcPositions.end())
		{
			const Npc* npc = NpcManager::GetNpc(npcIt->second);
			if (npc)
			{
				ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.5f, 1.0f),
					"NPC: %s", npc->name.c_str());
				ImGui::Text("Press Space to talk");
			}
		}
	}

	// Show quest hint
	if (m_hasCharacter)
	{
		int32_t activeCount = 0;
		for (const auto& qe : m_character.GetQuestLog())
		{
			if (qe.status == QuestStatus::Active) activeCount++;
		}
		if (activeCount > 0)
		{
			ImGui::Text("Quests: %d active (J)", activeCount);
		}
	}

	if (ImGui::Button("Back to Menu"))
	{
		m_machine.ReplaceState("MainMenu");
	}
	ImGui::End();

	// ---- Position info ----
	ImGui::SetNextWindowPos(ImVec2(0, 210), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(260, 50), ImGuiCond_FirstUseEver);
	ImGui::Begin("Position");
	ImGui::Text("Grid: [%d, %d]", gp.row, gp.col);
	ImGui::Text("Facing: %s",
		m_camera.Facing() == Direction::North ? "North" :
		m_camera.Facing() == Direction::East  ? "East"  :
		m_camera.Facing() == Direction::South ? "South" : "West");
	ImGui::End();

	// ---- Compass (step 262) ----
	renderCompass();

	// ---- Minimap ----
	renderMinimap();

	// ---- Location popup (steps 251-256) ----
	if (m_showLocationPopup)
	{
		renderLocationPopup();
	}

	// ---- NPC Dialogue (Space near NPC) ----
	if (m_showDialogue)
	{
		renderNpcDialogue();
	}

	// ---- Trade window ----
	if (m_showTrade)
	{
		renderTradeWindow();
	}

	// ---- Quest journal (J key) ----
	if (m_showQuestJournal)
	{
		renderQuestJournal();
	}

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
		ImGui::SetNextWindowPos(ImVec2(0, 270), ImGuiCond_FirstUseEver, ImVec2(0, 0));
		ImGui::SetNextWindowSize(ImVec2(260, 160), ImGuiCond_FirstUseEver);
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
		ImGui::Text("Time: %02d:00", m_dayTime);
		ImGui::Text("Ambient: %.2f", m_ambientLight);
		ImGui::Text("Night: %s", m_isNight ? "yes" : "no");
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

	// Legend (step 261)
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

	ImGui::Text("Select destination (requires road connection, costs %d gold):", FAST_TRAVEL_COST);
	ImGui::Separator();

	GridPosition currentPos = m_camera.GetGridPosition();
	bool anyAvailable = false;

	// Check if player has enough gold
	auto& inv = m_character.GetInventory();
	int32_t totalGold = 0;
	for (size_t i = 0; i < inv.Size(); ++i)
	{
		const Item* item = inv.Get(i);
		if (item && item->type == ItemType::Gold)
		{
			totalGold += item->value;
		}
	}

	ImGui::Text("Your gold: %d", totalGold);
	ImGui::Separator();

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
		bool canAfford = totalGold >= FAST_TRAVEL_COST;

		if (!canAfford)
		{
			ImGui::BeginDisabled();
		}

		if (ImGui::Button(loc.name.c_str(), ImVec2(200, 30)))
		{
			if (onRoad && canAfford)
			{
				// Deduct gold
				int32_t toRemove = FAST_TRAVEL_COST;
				for (size_t i = 0; i < inv.Size() && toRemove > 0; )
				{
					Item* item = inv.Get(i);
					if (item && item->type == ItemType::Gold)
					{
						int32_t stackValue = item->value;
						if (stackValue <= toRemove)
						{
							toRemove -= stackValue;
							inv.Remove(i);
						}
						else
						{
							item->value = stackValue - toRemove;
							toRemove = 0;
							++i;
						}
					}
					else
					{
						++i;
					}
				}

				m_camera.SetGridPosition(locPos, Direction::North);
				m_camera.SnapToGrid();
				m_overworld.MarkVisited(locPos);
				m_character.GetPosition() = locPos;
				m_showFastTravel = false;
				Logger::Info("Fast travelled to " + loc.name);
			}
		}

		if (!canAfford)
		{
			ImGui::EndDisabled();
		}

		if (onRoad)
		{
			ImGui::SameLine();
			if (canAfford)
			{
				ImGui::Text("(%d, %d) - %s", loc.position.row, loc.position.col,
					loc.description.c_str());
			}
			else
			{
				ImGui::TextDisabled("(need %d gold)", FAST_TRAVEL_COST);
			}
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

//=============================================================================
// Compass (step 262): shows direction to nearest known location
//=============================================================================

void OverworldState::renderCompass() noexcept
{
	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 ws = viewport->WorkSize;

	ImGui::SetNextWindowPos(ImVec2(ws.x - 210, 210), ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(200, 80), ImGuiCond_FirstUseEver);
	ImGui::Begin("Compass", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

	GridPosition playerPos = m_camera.GetGridPosition();

	// Find nearest known location
	const OverworldLocation* nearest = nullptr;
	float nearestDist = 1e9f;
	for (const auto& loc : m_overworld.Locations())
	{
		GridPosition locPos = {loc.position.row, loc.position.col, 0};
		if (!m_overworld.IsVisited(locPos)) continue;

		int32_t dr = loc.position.row - playerPos.row;
		int32_t dc = loc.position.col - playerPos.col;
		float dist = std::sqrt(static_cast<float>(dr * dr + dc * dc));
		if (dist < nearestDist)
		{
			nearestDist = dist;
			nearest = &loc;
		}
	}

	if (!nearest)
	{
		ImGui::Text("No known locations");
		ImGui::End();
		return;
	}

	// Calculate bearing relative to player facing
	static constexpr float PI = static_cast<float>(std::numbers::pi);
	static constexpr float HALF_PI = static_cast<float>(std::numbers::pi_v<long double> / 2.0L);
	static constexpr float TWO_PI = static_cast<float>(std::numbers::pi * 2.0);

	int32_t dr = nearest->position.row - playerPos.row;
	int32_t dc = nearest->position.col - playerPos.col;

	// Angle in world space (row = -Z, col = +X)
	float worldAngle = std::atan2(static_cast<float>(dc), static_cast<float>(-dr));

	// Player facing angle
	float facingAngle = 0.0f;
	switch (m_camera.Facing())
	{
		case Direction::North: facingAngle = 0.0f; break;
		case Direction::East:  facingAngle = -HALF_PI; break;
		case Direction::South: facingAngle = PI; break;
		case Direction::West:  facingAngle = HALF_PI; break;
	}

	float relativeAngle = worldAngle - facingAngle;
	// Normalize to [-PI, PI]
	while (relativeAngle > PI)  relativeAngle -= TWO_PI;
	while (relativeAngle < -PI) relativeAngle += TWO_PI;

	// Draw compass
	ImDrawList* draw = ImGui::GetWindowDrawList();
	const ImVec2 avail = ImGui::GetContentRegionAvail();
	const ImVec2 center = ImGui::GetCursorScreenPos() + ImVec2(avail.x * 0.5f, avail.y * 0.5f);
	const float compassRadius = 28.0f;

	// Outer circle
	draw->AddCircle(center, compassRadius, IM_COL32(120, 120, 120, 200), 20, 1.5f);

	// Direction labels
	draw->AddText(center + ImVec2(-4.0f, -compassRadius - 12.0f),
		IM_COL32(200, 200, 200, 220), "N");
	draw->AddText(center + ImVec2(compassRadius + 4.0f, -5.0f),
		IM_COL32(160, 160, 160, 180), "E");
	draw->AddText(center + ImVec2(-compassRadius - 14.0f, -5.0f),
		IM_COL32(160, 160, 160, 180), "W");
	draw->AddText(center + ImVec2(-4.0f, compassRadius + 2.0f),
		IM_COL32(160, 160, 160, 180), "S");

	// Arrow pointing to nearest location
	ImVec2 arrowEnd = center + ImVec2(
		std::sin(relativeAngle) * compassRadius * 0.7f,
		-std::cos(relativeAngle) * compassRadius * 0.7f);

	draw->AddCircleFilled(center, 3.0f, IM_COL32(255, 215, 0, 220), 6);
	draw->AddLine(center, arrowEnd, IM_COL32(255, 215, 0, 220), 2.5f);

	// Location label
	ImVec2 labelPos = center + ImVec2(-compassRadius, compassRadius + 16.0f);
	draw->AddText(labelPos, IM_COL32(255, 215, 0, 200), nearest->name.c_str());

	// Distance
	char distStr[32];
	std::snprintf(distStr, sizeof(distStr), "(%.0f cells)", nearestDist);
	draw->AddText(labelPos + ImVec2(0.0f, 14.0f),
		IM_COL32(160, 160, 160, 180), distStr);

	ImGui::Dummy(ImVec2(avail.x, avail.y));
	ImGui::End();
}

//=============================================================================
// Location popup (steps 251-256)
//=============================================================================

void OverworldState::renderLocationPopup() noexcept
{
	const OverworldLocation* loc = nullptr;
	for (const auto& l : m_overworld.Locations())
	{
		if (l.id == m_activeLocationId)
		{
			loc = &l;
			break;
		}
	}

	if (!loc) return;

	const ImGuiViewport* viewport = ImGui::GetMainViewport();
	const ImVec2 ws = viewport->WorkSize;

	ImGui::SetNextWindowPos(ImVec2(ws.x * 0.5f - 200.0f, ws.y * 0.5f - 120.0f),
		ImGuiCond_FirstUseEver, ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2(400, 240), ImGuiCond_FirstUseEver);

	ImGui::Begin(loc->name.c_str(), &m_showLocationPopup,
		ImGuiWindowFlags_AlwaysAutoResize);

	ImGui::TextWrapped("%s", loc->description.c_str());
	ImGui::Separator();

	switch (loc->type)
	{
		case LocationType::Town:
		{
			ImGui::Text("Welcome to %s!", loc->name.c_str());
			ImGui::Text("HP: %d/%d  MP: %d/%d",
				m_character.GetHp(), m_character.GetMaxHp(),
				m_character.GetMp(), m_character.GetMaxMp());

			// Check gold
			int32_t totalGold = 0;
			auto& inv = m_character.GetInventory();
			for (size_t i = 0; i < inv.Size(); ++i)
			{
				const Item* item = inv.Get(i);
				if (item && item->type == ItemType::Gold)
					totalGold += item->value;
			}
			ImGui::Text("Gold: %d", totalGold);

			if (ImGui::Button("Rest (10 gold) - Full Heal", ImVec2(250, 0)))
			{
				processTownInteraction();
			}
			ImGui::SameLine();
			if (ImGui::Button("Leave", ImVec2(100, 0)))
			{
				m_showLocationPopup = false;
			}
			break;
		}

		case LocationType::DungeonEntrance:
		{
			ImGui::Text("An ancient dungeon entrance looms before you.");
			ImGui::Text("Level %d party recommended.", m_character.GetLevel());

			if (ImGui::Button("Enter Dungeon", ImVec2(200, 0)))
			{
				processDungeonEntranceInteraction();
				// Note: popup closed inside the function
			}
			ImGui::SameLine();
			if (ImGui::Button("Leave", ImVec2(100, 0)))
			{
				m_showLocationPopup = false;
			}
			break;
		}

		case LocationType::Shrine:
		{
			if (m_shrineUsed)
			{
				ImGui::Text("The shrine's power has been spent.");
				if (ImGui::Button("Leave", ImVec2(100, 0)))
				{
					m_showLocationPopup = false;
				}
			}
			else
			{
				ImGui::Text("A sacred shrine radiates divine energy.");
				ImGui::Text("HP: %d/%d  MP: %d/%d",
					m_character.GetHp(), m_character.GetMaxHp(),
					m_character.GetMp(), m_character.GetMaxMp());
				ImGui::TextColored(ImVec4(0, 1, 0, 1),
					"Blessing: +2 ATK, +2 AC for 50 turns");

				if (ImGui::Button("Pray (Restore HP/MP + Blessing)", ImVec2(300, 0)))
				{
					processShrineInteraction();
					m_showLocationPopup = false;
				}
				ImGui::SameLine();
				if (ImGui::Button("Leave", ImVec2(100, 0)))
				{
					m_showLocationPopup = false;
				}
			}
			break;
		}

		case LocationType::Camp:
		{
			ImGui::Text("A campfire crackles warmly.");
			ImGui::Text("HP: %d/%d  MP: %d/%d",
				m_character.GetHp(), m_character.GetMaxHp(),
				m_character.GetMp(), m_character.GetMaxMp());

			// Check food
			int32_t foodCount = 0;
			auto& inv = m_character.GetInventory();
			for (size_t i = 0; i < inv.Size(); ++i)
			{
				const Item* item = inv.Get(i);
				if (item && item->type == ItemType::Food && item->expirationTurns != 0)
					foodCount++;
			}

			ImGui::Text("Fresh food available: %d", foodCount);

			if (foodCount > 0)
			{
				if (ImGui::Button("Rest (use 1 food) - 50% Heal", ImVec2(250, 0)))
				{
					processCampInteraction();
					m_showLocationPopup = false;
				}
				ImGui::SameLine();
			}
			if (ImGui::Button("Leave", ImVec2(100, 0)))
			{
				m_showLocationPopup = false;
			}
			break;
		}
	}

	ImGui::End();
}

//=============================================================================
// Day/night indicator (step 265)
//=============================================================================

void OverworldState::renderDayNightIndicator() noexcept
{
	ImGui::Separator();

	// Sun icon during day, moon icon during night
	const char* icon = m_isNight ? "\xce\x9c" : "\xce\xa3"; // Moon/Sun approximation
	ImVec4 color = m_isNight
		? ImVec4(0.4f, 0.4f, 0.8f, 1.0f)
		: ImVec4(1.0f, 0.9f, 0.3f, 1.0f);

	ImGui::TextColored(color, "%s Time: %02d:00  (Ambient: %.0f%%)",
		icon, m_dayTime, m_ambientLight * 100.0f);

	if (m_isNight)
	{
		ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.8f, 1.0f),
			"Night: view -3, encounters x2");
	}
}
