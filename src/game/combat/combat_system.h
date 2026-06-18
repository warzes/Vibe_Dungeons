#pragma once

#include "game/combat/character.h"
#include "game/combat/monster.h"
#include "game/combat/monster_manager.h"
#include "game/data/skill_manager.h"
#include "game/combat/monster_group.h"
#include "game/dungeon/dungeon.h"

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

	// Ranged attack (steps 133-134, 138)
	[[nodiscard]] AttackResult RangedAttack(
		const Character& attacker,
		Monster& defender,
		const Dungeon& dungeon,
		bool pointBlank = false  // step 138: penalty if adjacent
	);

	[[nodiscard]] AttackResult MonsterRangedAttack(
		const Monster& attacker,
		Character& defender,
		const Dungeon& dungeon
	);

	// AoE targeting (steps 139-146)
	[[nodiscard]] AoeTarget CalculateAoeTarget(
		GridPosition center,
		TargetingMode mode,
		int32_t radius,
		const MonsterManager& monsterManager,
		const Dungeon& dungeon
	);

	void ApplyAoeDamage(
		const AoeTarget& target,
		const Character& attacker,
		MonsterManager& monsterManager,
		int32_t damageMin,
		int32_t damageMax,
		const std::string& element = "",
		int32_t dexSaveDc = 0  // step 146: DEX save for half damage
	);

	// Step 146: DEX save
	[[nodiscard]] static bool DexSave(const Monster& monster, int32_t dc);

	// Line of sight check (step 134)
	[[nodiscard]] static bool HasClearShot(
		const Dungeon& dungeon,
		GridPosition from,
		GridPosition to
	);

	[[nodiscard]] static int32_t ClassDamageBonus(const Character& attacker, bool behind = false);

	[[nodiscard]] static int32_t ClassAtkBonus(const Character& attacker, bool behind = false);

	[[nodiscard]] static int32_t ClassAcBonus(const Character& c);
};
