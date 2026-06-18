#include "stdafx.h"
#include "game/combat/spell_system.h"
#include "game/combat/character.h"
#include "game/combat/monster_manager.h"
#include "game/combat/dice.h"
#include "game/data/skill_manager.h"
#include "game/ui/combat_log.h"
#include "game/dungeon/dungeon.h"
#include "game/grid_position.h"
#include "game/direction.h"
#include "core/json_data_manager.h"
#include "game/combat/status_effect.h"
#include <cmath>

//=============================================================================

void SpellSystem::Init(
	Character& character,
	MonsterManager& monsterManager,
	CombatLog& combatLog,
	const Dungeon& dungeon
) noexcept
{
	m_character = &character;
	m_monsterManager = &monsterManager;
	m_combatLog = &combatLog;
	m_dungeon = &dungeon;
}

//=============================================================================

int32_t SpellSystem::GetCastingStat() const noexcept
{
	std::string cls = m_character->GetClass();
	if (cls == "mage")       { return m_character->GetIntel(); }
	if (cls == "war_priest") { return m_character->GetIntel(); }
	if (cls == "paladin")    { return m_character->GetStr(); }
	if (cls == "thief")      { return m_character->GetDex(); }
	if (cls == "barbarian")  { return m_character->GetStr(); }
	return m_character->GetIntel();
}

//=============================================================================

SpellTarget SpellSystem::AcquireTarget(const Skill& spell, int32_t range) noexcept
{
	SpellTarget result;
	glm::ivec2 fwd = DirectionToVec(m_character->GetFacing());
	GridPosition start = m_character->GetPosition();

	if (spell.type == "self" || spell.type == "heal")
	{
		result.position = start;
		return result;
	}

	// Determine targeting mode from spell type
	if (spell.type == "projectile" || spell.type == "beam")
	{
		result.targetingMode = (spell.type == "beam") ? TargetingMode::Beam : TargetingMode::Single;

		for (int32_t step = 1; step <= range; ++step)
		{
			GridPosition check = {
				start.row + fwd.x * step,
				start.col + fwd.y * step,
				start.floor
			};

			if (!m_dungeon->IsWalkable(check))
			{
				result.hitWall = true;
				result.position = check;
				break;
			}

			result.position = check;

			Monster* mon = m_monsterManager->At(check);
			if (mon && mon->alive)
			{
				result.hitMonsters.push_back(mon);
				if (spell.type == "projectile")
				{
					break;
				}
			}

			if (spell.type == "projectile" && !result.hitMonsters.empty())
			{
				break;
			}
		}

		if (result.hitMonsters.empty() && !result.hitWall)
		{
			result.outOfRange = (range > 0);
		}
	}
	else if (spell.type == "aoe")
	{
		result.targetingMode = TargetingMode::AoE_3x3;

		// Target cell is `range` cells in front (or closest walkable)
		for (int32_t step = 1; step <= range; ++step)
		{
			GridPosition check = {
				start.row + fwd.x * step,
				start.col + fwd.y * step,
				start.floor
			};

			if (!m_dungeon->IsWalkable(check))
			{
				break;
			}
			result.position = check;
		}

		if (result.position == start)
		{
			return result;
		}

		int32_t radius = spell.radius;
		if (radius < 1) { radius = 1; }
		result.radius = radius;

		for (int32_t dr = -radius; dr <= radius; ++dr)
		{
			for (int32_t dc = -radius; dc <= radius; ++dc)
			{
				GridPosition area = {
					result.position.row + dr,
					result.position.col + dc,
					result.position.floor
				};

				if (!Chunk::InBounds(area.row, area.col))
				{
					continue;
				}

				if (m_dungeon->GetCell(area).BlocksLineOfSight())
				{
					continue;
				}

				Monster* mon = m_monsterManager->At(area);
				if (mon && mon->alive)
				{
					result.hitMonsters.push_back(mon);
				}
			}
		}
	}
	else if (spell.type == "line")
	{
		result.targetingMode = TargetingMode::Line;

		// Line: all cells in a straight line from caster to max range
		for (int32_t step = 1; step <= range; ++step)
		{
			GridPosition check = {
				start.row + fwd.x * step,
				start.col + fwd.y * step,
				start.floor
			};

			if (!m_dungeon->IsWalkable(check))
			{
				result.hitWall = true;
				break;
			}

			result.position = check;

			Monster* mon = m_monsterManager->At(check);
			if (mon && mon->alive)
			{
				result.hitMonsters.push_back(mon);
			}
		}
	}

	return result;
}

//=============================================================================

int32_t SpellSystem::CalculateDamage(const Skill& spell, const Monster* target) const noexcept
{
	int32_t baseDamage = 0;
	if (spell.damageMin > 0 || spell.damageMax > 0)
	{
		baseDamage = Dice::Roll(spell.damageMin, spell.damageMax);
	}
	else if (spell.healMin > 0 || spell.healMax > 0)
	{
		baseDamage = -Dice::Roll(spell.healMin, spell.healMax);
	}

	double statBonus = static_cast<double>(GetCastingStat()) / 10.0;
	double levelBonus = static_cast<double>(m_character->GetLevel()) * 0.5;
	double total = (static_cast<double>(baseDamage) + statBonus + levelBonus) * spell.damageMult;

	if (target && spell.element == "holy")
	{
		bool isUndead = (target->typeId.find("undead") != std::string::npos)
			|| (target->name.find("Skeleton") != std::string::npos);
		if (isUndead)
		{
			double vsUndead = spell.id == "smite_undead" ? 2.0 : 1.5;
			total *= vsUndead;
		}
	}

	return static_cast<int32_t>(std::round(total));
}

//=============================================================================

void SpellSystem::ApplyStatusEffect(
	const Skill& spell,
	Monster& monster,
	const std::string& sourceName) noexcept
{
	// Check if spell has a status effect defined in JSON
	// The spell JSON can have a "statusEffect" field:
	// "statusEffect": {"id": "burn", "chance": 0.5, "duration": 3}
	//
	// Or we can map spell IDs to effects directly

	// Look up status effect info from spells.json
	const json& spellJson = JsonDataManager::Instance().GetSpellData(spell.id);
	if (spellJson.is_null() || !spellJson.contains("statusEffect"))
	{
		return;
	}

	const json& se = spellJson["statusEffect"];
	std::string effectId = se.value("id", std::string());
	if (effectId.empty())
	{
		return;
	}

	double chance = se.value("chance", 1.0);
	int32_t duration = se.value("duration", 3);

	// Roll for application
	if (Dice::Roll(1, 100) > static_cast<int32_t>(chance * 100.0))
	{
		return;
	}

	m_statusSystem.ApplyEffect(
		monster.activeEffects,
		effectId,
		sourceName,
		duration,
		monster.resistances,
		monster.immunities
	);
}

void SpellSystem::CleanseTarget(Monster& monster) noexcept
{
	m_statusSystem.RemoveAllNegative(monster.activeEffects);
	m_combatLog->Add("Cleansed " + monster.name + " of all negative effects.",
		glm::vec3(0.2f, 1.0f, 0.2f));
}

void SpellSystem::CleanseCharacter() noexcept
{
	m_statusSystem.RemoveAllNegative(m_character->GetActiveEffects());
	m_combatLog->Add("Cleansed of all negative effects.",
		glm::vec3(0.2f, 1.0f, 0.2f));
}

void SpellSystem::ApplySpell(const Skill& spell, const SpellTarget& target) noexcept
{
	if (spell.type == "self" || spell.type == "heal")
	{
		int32_t healAmount = CalculateDamage(spell);
		int32_t actualHeal = std::abs(healAmount);
		m_character->Heal(actualHeal);
		m_combatLog->Add(spell.name + " heals for " + std::to_string(actualHeal) + " HP.",
			glm::vec3(0.2f, 1.0f, 0.2f));
		m_character->SpendMp(spell.mpCost);
		return;
	}

	// Projectile/beam/aoe: apply to hit monsters
	for (Monster* mon : target.hitMonsters)
	{
		if (!mon || !mon->alive)
		{
			continue;
		}

		int32_t dmg = CalculateDamage(spell, mon);
		mon->hp -= dmg;
		m_combatLog->Add(spell.name + " hits " + mon->name + " for " + std::to_string(dmg) + " damage!",
			glm::vec3(1.0f, 0.6f, 0.2f));

		if (mon->hp <= 0)
		{
			mon->hp = 0;
			mon->alive = false;
			m_combatLog->Add(mon->name + " dies!", glm::vec3(0.2f, 1.0f, 0.2f));
		}
		else
		{
			// Apply status effect (steps 151-152)
			ApplyStatusEffect(spell, *mon, spell.name);
			m_statusSystem.AdvanceTurns(mon->activeEffects);
		}
	}

	if (target.hitMonsters.empty() && spell.type != "self")
	{
		if (target.hitWall)
		{
			m_combatLog->Add(spell.name + " hits the wall!", glm::vec3(0.6f));
		}
		else
		{
			m_combatLog->Add(spell.name + " fizzles!", glm::vec3(0.6f, 0.3f, 0.6f));
		}
	}
	else if (!target.hitMonsters.empty())
	{
		m_character->SpendMp(spell.mpCost);
	}
}
