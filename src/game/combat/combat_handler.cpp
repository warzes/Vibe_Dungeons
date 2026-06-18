#include "stdafx.h"
#include "game/combat/combat_handler.h"
#include "engine/resource_manager.h"
#include "engine/renderer/shader.h"
#include "engine/renderer/texture.h"
#include "engine/renderer/material.h"
#include "engine/renderer/renderer.h"
#include "engine/renderer/camera.h"
#include "engine/renderer/mesh.h"
#include "core/logger.h"
#include "core/json_data_manager.h"
#include "game/combat/character.h"
#include "game/combat/monster_manager.h"
#include "game/combat/monster_renderer.h"
#include "game/combat/combat_system.h"
#include "game/combat/spell_system.h"
#include "game/combat/item_handler.h"
#include "game/data/monster_factory.h"
#include "game/data/item_factory.h"
#include "game/data/experience_system.h"
#include "game/data/skill_manager.h"
#include "game/ui/combat_log.h"
#include "game/turn_manager.h"
#include "game/dungeon/dungeon.h"
#include "game/direction.h"
#include "game/grid_position.h"

//=============================================================================

void CombatHandler::Init(
	Character& character,
	MonsterManager& monsterManager,
	MonsterRenderer& monsterRenderer,
	CombatSystem& combatSystem,
	CombatLog& combatLog,
	TurnManager& turnManager,
	Camera& camera,
	Dungeon& dungeon,
	ResourceManager& resources,
	Shader& dungeonShader,
	bool& pendingLevelUp,
	ItemHandler& itemHandler
) noexcept
{
	m_character = &character;
	m_monsterManager = &monsterManager;
	m_monsterRenderer = &monsterRenderer;
	m_combatSystem = &combatSystem;
	m_combatLog = &combatLog;
	m_turnManager = &turnManager;
	m_camera = &camera;
	m_dungeon = &dungeon;
	m_resources = &resources;
	m_dungeonShader = &dungeonShader;
	m_pendingLevelUp = &pendingLevelUp;
	m_itemHandler = &itemHandler;
}

//=============================================================================

void CombatHandler::LoadMonsterTextures() noexcept
{
	const json& monstersJson = JsonDataManager::Instance().AllMonsters();
	for (const auto& entry : monstersJson)
	{
		std::string typeId = entry.value("id", std::string());
		if (typeId.empty()) { continue; }

		std::string texPath = entry.value("texture", std::string());
		if (texPath.empty()) { continue; }

		std::string texKey = "tex_monster_" + typeId;
		std::string matKey = "mat_monster_" + typeId;

		Texture* tex = m_resources->LoadTexture(texKey.c_str(), texPath.c_str(), false);
		if (!tex)
		{
			Logger::Warn(std::string("Failed to load texture for monster: ") + typeId + " at " + texPath);
			continue;
		}

		Material* mat = m_resources->GetOrCreateMaterial(matKey.c_str(), *m_dungeonShader, *tex);
		if (!mat)
		{
			Logger::Warn(std::string("Failed to create material for monster: ") + typeId);
			continue;
		}

		m_monsterTextures[typeId] = tex;
		m_monsterMaterials[typeId] = mat;
	}
}

//=============================================================================

void CombatHandler::SpawnDefault() noexcept
{
	{
		Monster skelly = MonsterFactory::Create("skeleton", 1);
		skelly.position = {16, 17, 0};
		skelly.facing = Direction::South;
		m_monsterManager->Spawn(std::move(skelly));
	}
	{
		Monster slime = MonsterFactory::Create("slime", 1);
		slime.position = {13, 17, 0};
		slime.facing = Direction::South;
		m_monsterManager->Spawn(std::move(slime));
	}
}

//=============================================================================

void CombatHandler::PerformCombat() noexcept
{
	// Check if equipped weapon is ranged (steps 131-133)
	if (HasRangedWeaponEquipped())
	{
		PerformRangedAttack();
		return;
	}

	Monster* target = m_monsterManager->FindInFront(
		m_camera->GetGridPosition(),
		m_camera->Facing()
	);

	if (!target)
	{
		m_combatLog->Add("Nothing to attack.", glm::vec3(0.6f));
		return;
	}

	bool behind = (DirectionToTarget(target->position, m_character->GetPosition()) == Opposite(target->facing));
	AttackResult playerResult = m_combatSystem->MeleeAttack(*m_character, *target, behind);

	// Step 180: weapon loses durability on use
	reduceWeaponDurability();

	if (playerResult.critical)
	{
		m_combatLog->Add("Critical hit!", glm::vec3(1.0f, 0.2f, 0.2f));
	}

	if (playerResult.hit)
	{
		std::string msg = "Hero hits " + target->name + " for " +
			std::to_string(playerResult.damage) + " damage!";
		m_combatLog->Add(msg, glm::vec3(1.0f));
	}
	else
	{
		std::string msg = "Hero missed " + target->name + "!";
		m_combatLog->Add(msg, glm::vec3(0.7f, 0.7f, 0.7f));
	}

	if (playerResult.killed)
	{
		onKill(*target);
		return;
	}

	// Monster attacks back (only adjacent melee)
	if (target && target->alive)
	{
		AttackResult monsterResult = m_combatSystem->MonsterMeleeAttack(*target, *m_character);

		if (monsterResult.critical)
		{
			m_combatLog->Add("Critical hit!", glm::vec3(1.0f, 0.2f, 0.2f));
		}

		if (monsterResult.hit)
		{
			std::string msg = target->name + " hits Hero for " +
				std::to_string(monsterResult.damage) + " damage!";
			m_combatLog->Add(msg, glm::vec3(1.0f, 0.8f, 0.2f));
		}
		else
		{
			std::string msg = target->name + " missed Hero!";
			m_combatLog->Add(msg, glm::vec3(0.7f, 0.7f, 0.7f));
		}
	}

	if (m_character->GetHp() <= 0)
	{
		m_combatLog->Add("Hero has fallen!", glm::vec3(1.0f, 0.0f, 0.0f));
		m_turnManager->SetGameMode(GameMode::GameOver);
		return;
	}

	m_turnManager->SetGameMode(GameMode::TurnWaiting);
}

//=============================================================================

void CombatHandler::ProcessActionSlot(int32_t slotIndex) noexcept
{
	if (slotIndex < 0 || slotIndex >= Character::NUM_ACTION_SLOTS)
	{
		return;
	}

	auto& slot = m_character->GetActionSlots()[slotIndex];
	if (slot.id.empty())
	{
		return;
	}

	if (slot.cooldownRemaining > 0)
	{
		m_combatLog->Add("Ability on cooldown.", glm::vec3(0.8f, 0.4f, 0.4f));
		return;
	}

	UseAbility(slot.id);

	Skill s = SkillManager::GetSkill(slot.id);
	slot.cooldownRemaining = s.cooldown;
}

//=============================================================================

void CombatHandler::UseAbility(const std::string& abilityId) noexcept
{
	if (m_turnManager->GetGameMode() != GameMode::Exploring
		&& m_turnManager->GetGameMode() != GameMode::TurnWaiting)
	{
		return;
	}

	Skill skill = SkillManager::GetSkill(abilityId);
	if (skill.id.empty())
	{
		m_combatLog->Add("Unknown ability!", glm::vec3(1.0f, 0.2f, 0.2f));
		return;
	}

	if (skill.mpCost > m_character->GetMp())
	{
		m_combatLog->Add("Not enough MP!", glm::vec3(0.8f, 0.4f, 0.8f));
		return;
	}

	if (skill.type == "self" || skill.type == "heal")
	{
		useSelfAbility(skill);
		return;
	}

	if (skill.type == "melee")
	{
		useMeleeAbility(skill);
		return;
	}

	if (skill.type == "movement")
	{
		useMovementAbility(skill);
		return;
	}

	if (skill.type == "ranged" || skill.type == "aoe")
	{
		useRangedAbility(skill);
		return;
	}

	m_combatLog->Add("Cannot use that ability here.", glm::vec3(0.8f, 0.4f, 0.2f));
}

//=============================================================================

bool CombatHandler::HasRangedWeaponEquipped() const noexcept
{
	const Item* weapon = m_character->GetEquipment().Get(EquipmentSlot::Weapon);
	if (!weapon)
	{
		return false;
	}

	// Check if weapon has "ranged" tag in weapon_types.json
	const json& weaponTypes = JsonDataManager::Instance().AllWeaponTypes();
	for (const auto& wt : weaponTypes)
	{
		std::string id = wt.value("id", std::string());
		if (id.empty()) { continue; }

		// Match by checking if item name contains the weapon type name, or by subtype
		if (weapon->name.find(wt.value("name", id)) != std::string::npos
			|| weapon->name.find(id) != std::string::npos)
		{
			int32_t range = wt.value("range", 1);
			if (range > 1)
			{
				return true;
			}
		}
	}

	return false;
}

int32_t CombatHandler::GetEquippedWeaponRange() const noexcept
{
	const Item* weapon = m_character->GetEquipment().Get(EquipmentSlot::Weapon);
	if (!weapon)
	{
		return 1;
	}

	const json& weaponTypes = JsonDataManager::Instance().AllWeaponTypes();
	for (const auto& wt : weaponTypes)
	{
		std::string id = wt.value("id", std::string());
		if (id.empty()) { continue; }

		if (weapon->name.find(wt.value("name", id)) != std::string::npos
			|| weapon->name.find(id) != std::string::npos)
		{
			return wt.value("range", 1);
		}
	}

	return 1;
}

bool CombatHandler::hasAmmo() const noexcept
{
	// Check inventory for arrows or bolts
	const Inventory& inv = m_character->GetInventory();
	for (size_t i = 0; i < inv.Size(); ++i)
	{
		const Item* item = inv.Get(i);
		if (item && (item->type == ItemType::Arrow || item->type == ItemType::Bolt))
		{
			return true;
		}
	}
	return false;
}

void CombatHandler::consumeAmmo() noexcept
{
	Inventory& inv = m_character->GetInventory();
	for (size_t i = 0; i < inv.Size(); ++i)
	{
		const Item* item = inv.Get(i);
		if (item && (item->type == ItemType::Arrow || item->type == ItemType::Bolt))
		{
			inv.Remove(i);
			return;
		}
	}
}

void CombatHandler::PerformRangedAttack() noexcept
{
	// Step 133: target at distance
	Monster* target = nullptr;
	int32_t range = GetEquippedWeaponRange();
	glm::ivec2 fwd = DirectionToVec(m_character->GetFacing());
	GridPosition start = m_character->GetPosition();

	// Find first monster in line up to weapon range
	for (int32_t step = 1; step <= range; ++step)
	{
		GridPosition check = {start.row + fwd.x * step, start.col + fwd.y * step, start.floor};

		if (!m_dungeon->IsWalkable(check))
		{
			break; // blocked by wall
		}

		target = m_monsterManager->At(check);
		if (target && target->alive)
		{
			break;
		}
	}

	if (!target)
	{
		m_combatLog->Add("No target in ranged range.", glm::vec3(0.8f, 0.4f, 0.2f));
		return;
	}

	// Step 135: check ammo
	const Item* weapon = m_character->GetEquipment().Get(EquipmentSlot::Weapon);
	bool needsAmmo = false;
	if (weapon)
	{
		std::string wname = weapon->name;
		if (wname.find("Bow") != std::string::npos) { needsAmmo = true; }
		if (wname.find("Crossbow") != std::string::npos) { needsAmmo = true; }
	}
	if (needsAmmo && !hasAmmo())
	{
		m_combatLog->Add("No ammunition!", glm::vec3(0.8f, 0.4f, 0.2f));
		return;
	}

	// Step 138: check if point-blank (adjacent monster)
	bool pointBlank = false;
	GridPosition adjacent = {start.row + fwd.x, start.col + fwd.y, start.floor};
	if (target->position == adjacent)
	{
		pointBlank = true;
	}

	AttackResult result = m_combatSystem->RangedAttack(*m_character, *target, *m_dungeon, pointBlank);

	if (result.hit)
	{
		if (needsAmmo) { consumeAmmo(); }

		if (result.critical)
		{
			m_combatLog->Add("Critical hit!", glm::vec3(1.0f, 0.2f, 0.2f));
		}

		if (pointBlank)
		{
			m_combatLog->Add("Point-blank shot: -2 atk penalty applied.",
				glm::vec3(0.8f, 0.6f, 0.2f));
		}

		m_combatLog->Add("Hero's ranged attack hits " + target->name + " for " +
			std::to_string(result.damage) + " damage!", glm::vec3(1.0f, 0.6f, 0.2f));
	}
	else
	{
		m_combatLog->Add("Ranged attack missed!", glm::vec3(0.7f, 0.7f, 0.7f));
		if (result.damage == 0 && !result.hit)
		{
			m_combatLog->Add("Shot blocked by obstacle!", glm::vec3(0.8f, 0.4f, 0.2f));
		}
	}

	if (result.killed)
	{
		onKill(*target);
		return;
	}

	m_turnManager->SetGameMode(GameMode::TurnWaiting);
}

void CombatHandler::ProcessMonsterTurns() noexcept
{
	// Step 137: Ranged monsters attack from distance
	// Step 160: Group initiative
	std::vector<Monster> allMonsters = m_monsterManager->All();

	for (Monster& mon : allMonsters)
	{
		if (!mon.alive) { continue; }

		// Check if monster has ranged attack and player is in range
		if (mon.hasRangedAttack && mon.range > 1)
		{
			int32_t dist = std::abs(mon.position.row - m_character->GetPosition().row)
				+ std::abs(mon.position.col - m_character->GetPosition().col);

			if (dist <= mon.range && dist > 1)
			{
				// Line of sight check
				if (m_dungeon->HasLineOfSight(mon.position, m_character->GetPosition()))
				{
					AttackResult result = m_combatSystem->MonsterRangedAttack(mon, *m_character, *m_dungeon);

					if (result.hit)
					{
						m_combatLog->Add(mon.name + " shoots Hero for " +
							std::to_string(result.damage) + " damage!",
							glm::vec3(1.0f, 0.4f, 0.2f));
					}
					else
					{
						m_combatLog->Add(mon.name + "'s ranged attack missed!",
							glm::vec3(0.6f, 0.6f, 0.6f));
					}

					if (m_character->GetHp() <= 0)
					{
						m_combatLog->Add("Hero has fallen!", glm::vec3(1.0f, 0.0f, 0.0f));
						m_turnManager->SetGameMode(GameMode::GameOver);
						return;
					}
				}
			}
		}

		// Melee attack if adjacent
		int32_t dist = std::abs(mon.position.row - m_character->GetPosition().row)
			+ std::abs(mon.position.col - m_character->GetPosition().col);

		if (dist <= 1)
		{
			AttackResult result = m_combatSystem->MonsterMeleeAttack(mon, *m_character);

			if (result.hit)
			{
				m_combatLog->Add(mon.name + " hits Hero for " +
					std::to_string(result.damage) + " damage!",
					glm::vec3(1.0f, 0.8f, 0.2f));
			}
			else
			{
				m_combatLog->Add(mon.name + " missed Hero!",
					glm::vec3(0.6f, 0.6f, 0.6f));
			}

			if (m_character->GetHp() <= 0)
			{
				m_combatLog->Add("Hero has fallen!", glm::vec3(1.0f, 0.0f, 0.0f));
				m_turnManager->SetGameMode(GameMode::GameOver);
				return;
			}
		}
	}
}

void CombatHandler::SubmitRender(Renderer& renderer) noexcept
{
	m_monsterRenderer->Submit(renderer, *m_camera, m_monsterManager->All(), m_monsterMaterials);
}

//=============================================================================

void CombatHandler::useSelfAbility(const Skill& skill) noexcept
{
	m_character->SetMp(m_character->GetMp() - skill.mpCost);
	AttackResult result = m_combatSystem->UseAbility(*m_character, nullptr, skill);
	if (result.damage < 0)
	{
		m_combatLog->Add(skill.name + " restored " + std::to_string(-result.damage) + " HP.",
			glm::vec3(0.2f, 1.0f, 0.2f));
	}
	m_turnManager->SetGameMode(GameMode::TurnWaiting);
}

//=============================================================================

void CombatHandler::useMeleeAbility(const Skill& skill) noexcept
{
	glm::ivec2 fwd = DirectionToVec(m_character->GetFacing());
	GridPosition targetPos = {
		m_character->GetPosition().row + fwd.x,
		m_character->GetPosition().col + fwd.y,
		m_character->GetPosition().floor
	};

	Monster* target = m_monsterManager->At(targetPos);
	if (!target || !target->alive)
	{
		m_combatLog->Add("No enemy in front!", glm::vec3(0.8f, 0.4f, 0.2f));
		return;
	}
	m_character->SetMp(m_character->GetMp() - skill.mpCost);

	bool behind = (DirectionToTarget(target->position, m_character->GetPosition()) == Opposite(target->facing));
	AttackResult result = m_combatSystem->UseAbility(*m_character, target, skill, behind);

	if (result.hit)
	{
		std::string msg = skill.name + " hits " + target->name + " for " +
			std::to_string(result.damage) + " damage!";
		glm::vec3 color = result.critical ? glm::vec3(1.0f, 0.2f, 0.2f) : glm::vec3(1.0f, 1.0f, 0.2f);
		m_combatLog->Add(msg, color);
	}
	else
	{
		m_combatLog->Add(skill.name + " missed!", glm::vec3(0.6f, 0.6f, 0.6f));
	}

	if (result.killed)
	{
		onKill(*target);
	}

	m_turnManager->SetGameMode(GameMode::TurnWaiting);
}

//=============================================================================

void CombatHandler::useMovementAbility(const Skill& skill) noexcept
{
	glm::ivec2 fwd = DirectionToVec(m_character->GetFacing());
	GridPosition targetPos = {
		m_character->GetPosition().row + fwd.x,
		m_character->GetPosition().col + fwd.y,
		m_character->GetPosition().floor
	};

	if (m_dungeon->IsWalkable(targetPos) && !m_monsterManager->At(targetPos))
	{
		m_character->SetMp(m_character->GetMp() - skill.mpCost);
		m_character->GetPosition() = targetPos;
		m_camera->MoveForward();
		m_camera->SnapToGrid();
		m_character->GetPosition() = m_camera->GetGridPosition();
		m_character->SetFacing(m_camera->Facing());
		m_combatLog->Add(skill.name + " activated!", glm::vec3(0.6f, 0.3f, 1.0f));
	}
	else
	{
		m_combatLog->Add("Cannot shadow step there!", glm::vec3(0.8f, 0.4f, 0.2f));
	}
}

//=============================================================================

void CombatHandler::useRangedAbility(const Skill& skill) noexcept
{
	glm::ivec2 fwd = DirectionToVec(m_character->GetFacing());
	GridPosition targetPos = {
		m_character->GetPosition().row + fwd.x,
		m_character->GetPosition().col + fwd.y,
		m_character->GetPosition().floor
	};

	Monster* target = m_monsterManager->At(targetPos);
	if (!target || !target->alive)
	{
		m_combatLog->Add("No valid target!", glm::vec3(0.8f, 0.4f, 0.2f));
		return;
	}

	m_character->SetMp(m_character->GetMp() - skill.mpCost);
	bool behind = (DirectionToTarget(target->position, m_character->GetPosition()) == Opposite(target->facing));
	AttackResult result = m_combatSystem->UseAbility(*m_character, target, skill, behind);

	if (result.hit)
	{
		std::string msg = skill.name + " hits " + target->name + " for " +
			std::to_string(result.damage) + " damage!";
		m_combatLog->Add(msg, glm::vec3(1.0f, 0.6f, 0.2f));
	}
	else
	{
		m_combatLog->Add(skill.name + " missed!", glm::vec3(0.6f, 0.6f, 0.6f));
	}

	if (result.killed)
	{
		onKill(*target);
	}

	m_turnManager->SetGameMode(GameMode::TurnWaiting);
}

//=============================================================================

void CombatHandler::reduceWeaponDurability() noexcept
{
	Item* weapon = m_character->GetEquipment().Get(EquipmentSlot::Weapon);
	if (weapon && weapon->durability > 0)
	{
		weapon->durability -= 1;

		if (weapon->durability <= 0)
		{
			m_combatLog->Add(weapon->name + " breaks!", glm::vec3(1.0f, 0.0f, 0.0f));
			weapon->durability = 0;
		}
	}
}

//=============================================================================

void CombatHandler::onKill(Monster& target) noexcept
{
	m_combatLog->Add(target.name + " dies!", glm::vec3(0.2f, 1.0f, 0.2f));
	ExperienceSystem::AwardKill(*m_character, target);
	m_combatLog->Add("Gained " + std::to_string(target.xpReward) + " XP.",
		glm::vec3(0.3f, 0.6f, 1.0f));

	// ---- Loot drops (steps 178, 181) ----
	const json& monsterData = JsonDataManager::Instance().GetMonsterData(target.typeId);
	if (!monsterData.is_null() && monsterData.contains("drops"))
	{
		for (const auto& dropEntry : monsterData["drops"])
		{
			std::string itemId = dropEntry.value("itemId", "");
			double chance = dropEntry.value("chance", 0.0);
			int32_t minCount = dropEntry.value("minCount", 1);
			int32_t maxCount = dropEntry.value("maxCount", 1);

			// Roll for each potential drop
			double roll = static_cast<double>(rand()) / RAND_MAX;
			if (roll < chance)
			{
				int32_t count = minCount + (rand() % (maxCount - minCount + 1));
				for (int32_t di = 0; di < count; ++di)
				{
					Item dropItem = ItemFactory::CreateBase(itemId);
					ItemDrop drop;
					drop.item = dropItem;
					drop.position = target.position;
					m_itemHandler->Drops().push_back(std::move(drop));
				}
				m_combatLog->Add("Dropped " + ItemFactory::CreateBase(itemId).name + " x" + std::to_string(count) + "!",
					glm::vec3(1.0f, 0.8f, 0.2f));
			}
		}
	}

	// Essence drops based on monster element type (step 178)
	if (monsterData.contains("element") && !monsterData["element"].is_null())
	{
		std::string element = monsterData["element"].get<std::string>();
		std::string essenceId;
		if (element == "fire")       { essenceId = "fire_essence"; }
		else if (element == "ice")   { essenceId = "ice_essence"; }
		else if (element == "poison"){ essenceId = "poison_essence"; }
		else if (element == "holy")  { essenceId = "holy_essence"; }

		if (!essenceId.empty())
		{
			Item essence = ItemFactory::CreateBase(essenceId);
			ItemDrop drop;
			drop.item = essence;
			drop.position = target.position;
			m_itemHandler->Drops().push_back(std::move(drop));
			m_combatLog->Add("Dropped " + essence.name + "!", glm::vec3(1.0f, 0.5f, 1.0f));
		}
	}

	m_monsterManager->RemoveDead();
	*m_pendingLevelUp = ExperienceSystem::CheckLevelUp(*m_character);
}
