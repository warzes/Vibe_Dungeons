#include "stdafx.h"
#include "game/combat/combat_system.h"
#include "game/combat/dice.h"

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

	int32_t totalRoll = roll + attacker.GetAtkBonus() + ClassAtkBonus(attacker, behind);
	result.hit = (totalRoll >= defender.ac) || result.critical;

	if (result.hit)
	{
		if (result.critical)
		{
			result.damage = Dice::Roll(2, attacker.GetDamageMax()) + ClassDamageBonus(attacker, behind);
		}
		else
		{
			result.damage = Dice::Roll(attacker.GetDamageMin(), attacker.GetDamageMax()) + ClassDamageBonus(attacker, behind);
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

	int32_t effectiveAc = defender.GetAc() + ClassAcBonus(defender);
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

		int32_t totalRoll = roll + attacker.GetAtkBonus() + skill.atkBonus + ClassAtkBonus(attacker, behind);
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
					Dice::Roll(attacker.GetDamageMin(), attacker.GetDamageMax()) * skill.damageMult);
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
