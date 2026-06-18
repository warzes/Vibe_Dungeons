#include "stdafx.h"
#include "game/combat/combat_system.h"
#include "game/combat/dice.h"
#include "game/combat/monster_manager.h"

int32_t CombatSystem::ClassDamageBonus(const Character& attacker, bool behind)
{
	if (attacker.GetClass() == "barbarian") return 2;
	int32_t bonus = 0;
	if (attacker.GetClass() == "thief")
	{
		bonus = 1;
		if (behind) bonus += 1;
	}
	return bonus + attacker.GetDamageBonus();
}

int32_t CombatSystem::ClassAtkBonus(const Character& attacker, bool behind)
{
	int32_t bonus = 0;
	if (attacker.GetClass() == "thief")
	{
		bonus = 1;
		if (behind) bonus += 2;
	}
	if (attacker.GetClass() == "barbarian")
	{
		if (behind) bonus += 1;
	}
	return bonus;
}

int32_t CombatSystem::ClassAcBonus(const Character& c)
{
	if (c.GetClass() == "paladin")  return 2;
	if (c.GetClass() == "thief")    return 1;
	if (c.GetClass() == "mage")     return -2;
	return 0;
}

AttackResult CombatSystem::MeleeAttack(const Character& attacker, Monster& defender, bool behind)
{
	AttackResult result;

	int32_t roll = Dice::Roll(20);
	result.critical = (roll == 20);

	int32_t totalRoll = roll + attacker.GetEquippedAtkBonus() + ClassAtkBonus(attacker, behind);
	result.hit = (totalRoll >= defender.ac) || result.critical;

	if (result.hit)
	{
		if (result.critical)
		{
			result.damage = Dice::Roll(2, attacker.GetEquippedDamageMax()) + ClassDamageBonus(attacker, behind);
		}
		else
		{
			result.damage = Dice::Roll(attacker.GetEquippedDamageMin(), attacker.GetEquippedDamageMax()) + ClassDamageBonus(attacker, behind);
		}

		defender.hp -= result.damage;
		if (defender.hp <= 0)
		{
			defender.hp = 0;
			defender.alive = false;
			result.killed = true;
		}
	}

	return result;
}

AttackResult CombatSystem::MonsterMeleeAttack(const Monster& attacker, Character& defender)
{
	AttackResult result;

	int32_t roll = Dice::Roll(20);
	result.critical = (roll == 20);

	int32_t effectiveAc = defender.GetEquippedAc() + ClassAcBonus(defender);
	int32_t totalRoll = roll + attacker.atkBonus;
	result.hit = (totalRoll >= effectiveAc) || result.critical;

	if (result.hit)
	{
		int32_t damage = Dice::Roll(attacker.damageMin, attacker.damageMax);
		if (result.critical)
		{
			damage *= 2;
		}
		result.damage = damage;

		defender.TakeDamage(damage);
	}

	return result;
}

AttackResult CombatSystem::UseAbility(Character& attacker, Monster* defender,
	const Skill& skill, bool behind)
{
	AttackResult result;

	// Apply cooldown
	// (cooldown tracking is handled by PlayState)

	if (skill.type == "self" || skill.type == "heal")
	{
		int32_t heal = Dice::Roll(skill.healMin, skill.healMax);
		if (heal == 0 && skill.healMax == 0)
		{
			heal = Dice::Roll(4, 8);
		}
		attacker.Heal(heal);
		result.hit = true;
		result.damage = -heal;  // negative = healing
		return result;
	}

	if (!defender)
	{
		return result;
	}

	if (skill.type == "melee" || skill.type == "ranged")
	{
		int32_t roll = Dice::Roll(20);
		result.critical = (roll == 20);

		int32_t totalRoll = roll + attacker.GetEquippedAtkBonus() + skill.atkBonus + ClassAtkBonus(attacker, behind);
		result.hit = (totalRoll >= defender->ac) || result.critical;

		if (result.hit)
		{
			int32_t dmg = 0;
			if (skill.damageMin > 0 || skill.damageMax > 0)
			{
				dmg = Dice::Roll(skill.damageMin, skill.damageMax);
			}
			else
			{
				dmg = static_cast<int32_t>(
					Dice::Roll(attacker.GetEquippedDamageMin(), attacker.GetEquippedDamageMax()) * skill.damageMult);
			}
			dmg += ClassDamageBonus(attacker, behind);

			if (result.critical)
			{
				dmg *= 2;
			}

			result.damage = dmg;
			defender->hp -= dmg;
			if (defender->hp <= 0)
			{
				defender->hp = 0;
				defender->alive = false;
				result.killed = true;
			}
		}
	}

	return result;
}

//=============================================================================

AttackResult CombatSystem::RangedAttack(
	const Character& attacker,
	Monster& defender,
	const Dungeon& dungeon,
	bool pointBlank)
{
	AttackResult result;

	// Step 138: point-blank penalty (-2 atkBonus if adjacent)
	int32_t rangePenalty = pointBlank ? -2 : 0;

	// Check line of sight (step 134)
	if (!HasClearShot(dungeon, attacker.GetPosition(), defender.position))
	{
		return result; // blocked, miss
	}

	int32_t roll = Dice::Roll(20);
	result.critical = (roll == 20);

	int32_t totalRoll = roll + attacker.GetEquippedAtkBonus() + rangePenalty;
	result.hit = (totalRoll >= defender.ac) || result.critical;

	if (result.hit)
	{
		if (result.critical)
		{
			result.damage = Dice::Roll(2, attacker.GetEquippedDamageMax()) + ClassDamageBonus(attacker);
		}
		else
		{
			result.damage = Dice::Roll(attacker.GetEquippedDamageMin(), attacker.GetEquippedDamageMax()) + ClassDamageBonus(attacker);
		}

		defender.hp -= result.damage;
		if (defender.hp <= 0)
		{
			defender.hp = 0;
			defender.alive = false;
			result.killed = true;
		}
	}

	return result;
}

//=============================================================================

AttackResult CombatSystem::MonsterRangedAttack(
	const Monster& attacker,
	Character& defender,
	const Dungeon& dungeon)
{
	AttackResult result;

	// Check line of sight
	if (!HasClearShot(dungeon, attacker.position, defender.GetPosition()))
	{
		return result; // blocked
	}

	int32_t roll = Dice::Roll(20);
	result.critical = (roll == 20);

	int32_t effectiveAc = defender.GetEquippedAc() + ClassAcBonus(defender);
	int32_t totalRoll = roll + attacker.atkBonus;
	result.hit = (totalRoll >= effectiveAc) || result.critical;

	if (result.hit)
	{
		int32_t damage = Dice::Roll(attacker.rangedDamageMin, attacker.rangedDamageMax);
		if (result.critical)
		{
			damage *= 2;
		}
		result.damage = damage;

		defender.TakeDamage(damage);
	}

	return result;
}

//=============================================================================

AoeTarget CombatSystem::CalculateAoeTarget(
	GridPosition center,
	TargetingMode mode,
	int32_t radius,
	const MonsterManager& monsterManager,
	const Dungeon& dungeon)
{
	AoeTarget target;
	target.center = center;
	target.radius = radius;
	target.mode = mode;

	if (mode == TargetingMode::Single)
	{
		target.affectedCells.push_back(center);
		const Monster* mon = monsterManager.At(center);
		if (mon && mon->alive)
		{
			target.affectedMonsterIds.push_back(mon->id);
		}
		return target;
	}

	// AoE_3x3 or AoE_5x5
	int32_t halfSize = (mode == TargetingMode::AoE_3x3) ? 1 : 2;
	if (radius > 1) { halfSize = radius; }

	for (int32_t dr = -halfSize; dr <= halfSize; ++dr)
	{
		for (int32_t dc = -halfSize; dc <= halfSize; ++dc)
		{
			GridPosition cell = {center.row + dr, center.col + dc, center.floor};

			if (!Chunk::InBounds(cell.row, cell.col))
			{
				continue;
			}

			if (dungeon.GetCell(cell).BlocksLineOfSight())
			{
				continue;
			}

			target.affectedCells.push_back(cell);

			const Monster* mon = monsterManager.At(cell);
			if (mon && mon->alive)
			{
				target.affectedMonsterIds.push_back(mon->id);
			}
		}
	}

	return target;
}

//=============================================================================

void CombatSystem::ApplyAoeDamage(
	const AoeTarget& target,
	const Character& attacker,
	MonsterManager& monsterManager,
	int32_t damageMin,
	int32_t damageMax,
	const std::string& element,
	int32_t dexSaveDc)
{
	(void)attacker;
	(void)element;

	for (uint32_t monsterId : target.affectedMonsterIds)
	{
		Monster* mon = monsterManager.FindById(monsterId);
		if (!mon || !mon->alive)
		{
			continue;
		}

		int32_t dmg = Dice::Roll(damageMin, damageMax);

		// Step 146: DEX save for half damage
		if (dexSaveDc > 0 && DexSave(*mon, dexSaveDc))
		{
			dmg = std::max(1, dmg / 2);
		}

		mon->hp -= dmg;
		if (mon->hp <= 0)
		{
			mon->hp = 0;
			mon->alive = false;
		}
	}
}

//=============================================================================

bool CombatSystem::DexSave(const Monster& monster, int32_t dc)
{
	int32_t roll = Dice::Roll(20);
	// Approximate DEX bonus based on monster AC
	int32_t dexMod = (monster.ac - 10) / 2;
	return (roll + dexMod) >= dc;
}

//=============================================================================

bool CombatSystem::HasClearShot(
	const Dungeon& dungeon,
	GridPosition from,
	GridPosition to)
{
	return dungeon.HasLineOfSight(from, to);
}
