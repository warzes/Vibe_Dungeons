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

	if (spell.type == "self")
	{
		result.position = start;
		return result;
	}

	if (spell.type == "projectile" || spell.type == "beam")
	{
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
				// beam continues through monsters
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

	if (spell.type == "aoe")
	{
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

		for (int32_t dr = -radius; dr <= radius; ++dr)
		{
			for (int32_t dc = -radius; dc <= radius; ++dc)
			{
				GridPosition area = {
					result.position.row + dr,
					result.position.col + dc,
					result.position.floor
				};
				Monster* mon = m_monsterManager->At(area);
				if (mon && mon->alive)
				{
					result.hitMonsters.push_back(mon);
				}
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
