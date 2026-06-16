#pragma once

#include <cstdint>
#include "game/combat/character.h"
#include "game/combat/monster.h"

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
};
