#include "stdafx.h"
#include "game/combat/combat_system.h"
#include "game/combat/dice.h"

AttackResult CombatSystem::MeleeAttack(const Character& attacker, Monster& defender)
{
	AttackResult result;

	int32_t roll = Dice::Roll(20);
	result.critical = (roll == 20);

	int32_t totalRoll = roll + attacker.atkBonus;
	result.hit = (totalRoll >= defender.ac) || result.critical;

	if (result.hit)
	{
		if (result.critical)
		{
			result.damage = Dice::Roll(2, attacker.damageMax);
		}
		else
		{
			result.damage = Dice::Roll(attacker.damageMin, attacker.damageMax);
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

	int32_t totalRoll = roll + attacker.atkBonus;
	result.hit = (totalRoll >= defender.ac) || result.critical;

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
