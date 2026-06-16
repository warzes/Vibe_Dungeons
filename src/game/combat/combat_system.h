#pragma once

#include <cstdint>
#include <string>
#include "game/combat/character.h"
#include "game/combat/monster.h"
#include "game/data/skill_manager.h"

struct AttackResult final
{
	bool hit = false;
	int32_t damage = 0;
	bool critical = false;
	bool killed = false;
};

class CombatSystem final
{
public:
	[[nodiscard]] AttackResult MeleeAttack(const Character& attacker, Monster& defender, bool behind = false);

	[[nodiscard]] AttackResult MonsterMeleeAttack(const Monster& attacker, Character& defender);

	[[nodiscard]] AttackResult UseAbility(Character& attacker, Monster* defender,
		const Skill& skill, bool behind = false);

	[[nodiscard]] static int32_t ClassDamageBonus(const Character& attacker, bool behind = false);

	[[nodiscard]] static int32_t ClassAtkBonus(const Character& attacker, bool behind = false);

	[[nodiscard]] static int32_t ClassAcBonus(const Character& c);
};
