#pragma once

#include <string>
#include <vector>
#include "game/grid_position.h"
#include "game/combat/status_effect.h"
#include "game/combat/monster_group.h"

class Character;
class MonsterManager;
class CombatLog;
class Dungeon;
struct Monster;
struct Skill;

struct SpellTarget final
{
	GridPosition position;
	std::vector<Monster*> hitMonsters;
	bool hitWall = false;
	bool outOfRange = false;
	TargetingMode targetingMode = TargetingMode::Single;
	int32_t radius = 0;
};

class SpellSystem final
{
public:
	void Init(
		Character& character,
		MonsterManager& monsterManager,
		CombatLog& combatLog,
		const Dungeon& dungeon
	) noexcept;

	[[nodiscard]] SpellTarget AcquireTarget(const Skill& spell, int32_t range) noexcept;

	[[nodiscard]] int32_t CalculateDamage(const Skill& spell, const Monster* target = nullptr) const noexcept;

	void ApplySpell(const Skill& spell, const SpellTarget& target) noexcept;

	// Apply status effect from a spell to a monster (step 151-152)
	void ApplyStatusEffect(
		const Skill& spell,
		Monster& monster,
		const std::string& sourceName
	) noexcept;

	// Cleanse: remove all negative effects (step 153)
	void CleanseTarget(Monster& monster) noexcept;
	void CleanseCharacter() noexcept;

	[[nodiscard]] int32_t GetCastingStat() const noexcept;

	[[nodiscard]] StatusSystem& GetStatusSystem() noexcept { return m_statusSystem; }

private:
	Character* m_character = nullptr;
	MonsterManager* m_monsterManager = nullptr;
	CombatLog* m_combatLog = nullptr;
	const Dungeon* m_dungeon = nullptr;
	StatusSystem m_statusSystem;
};
