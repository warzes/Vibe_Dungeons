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
#include "game/data/monster_factory.h"
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
	bool& pendingLevelUp
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
		std::string msg = target->name + " dies!";
		m_combatLog->Add(msg, glm::vec3(0.2f, 1.0f, 0.2f));

		ExperienceSystem::AwardKill(*m_character, *target);
		m_combatLog->Add("Gained " + std::to_string(target->xpReward) + " XP.",
			glm::vec3(0.3f, 0.6f, 1.0f));

		m_monsterManager->RemoveDead();
		m_turnManager->SetGameMode(GameMode::TurnWaiting);
		*m_pendingLevelUp = ExperienceSystem::CheckLevelUp(*m_character);
		return;
	}

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

void CombatHandler::onKill(Monster& target) noexcept
{
	m_combatLog->Add(target.name + " dies!", glm::vec3(0.2f, 1.0f, 0.2f));
	ExperienceSystem::AwardKill(*m_character, target);
	m_combatLog->Add("Gained " + std::to_string(target.xpReward) + " XP.",
		glm::vec3(0.3f, 0.6f, 1.0f));
	m_monsterManager->RemoveDead();
	*m_pendingLevelUp = ExperienceSystem::CheckLevelUp(*m_character);
}
