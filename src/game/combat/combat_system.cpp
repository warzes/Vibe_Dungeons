#include "stdafx.h"
#include "game/combat/combat_system.h"
#include "game/combat/dice.h"

static int32_t classDamageBonus(const Character& attacker, bool behind = false)
{
	if (attacker.charClass == "barbarian") return 2;
	int32_t bonus = 0;
	if (attacker.charClass == "thief")
	{
		bonus = 1;
		if (behind) bonus += 1;
	}
	return bonus;
}

static int32_t classAtkBonus(const Character& attacker, bool behind = false)
{
	int32_t bonus = 0;
	if (attacker.charClass == "thief")
	{
		bonus = 1;
		if (behind) bonus += 1;
	}
	return bonus;
}

static int32_t classAcBonus(const Character& c)
{
	if (c.charClass == "paladin")  return 2;
	if (c.charClass == "thief")    return 1;
	if (c.charClass == "mage")     return -2;
	return 0;
}

AttackResult CombatSystem::MeleeAttack(const Character& attacker, Monster& defender, bool behind)
{
	AttackResult result;

	int32_t roll = Dice::Roll(20);
	result.critical = (roll == 20);

	int32_t totalRoll = roll + attacker.atkBonus + classAtkBonus(attacker, behind);
	result.hit = (totalRoll >= defender.ac) || result.critical;

	if (result.hit)
	{
		if (result.critical)
		{
			result.damage = Dice::Roll(2, attacker.damageMax) + classDamageBonus(attacker, behind);
		}
		else
		{
			result.damage = Dice::Roll(attacker.damageMin, attacker.damageMax) + classDamageBonus(attacker, behind);
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

	int32_t effectiveAc = defender.ac + classAcBonus(defender);
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

		defender.hp -= damage;
		if (defender.hp < 0)
		{
			defender.hp = 0;
		}
	}

	return result;
}
