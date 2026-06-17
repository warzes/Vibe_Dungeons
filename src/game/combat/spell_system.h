#pragma once

#include <string>
#include <vector>
#include "game/grid_position.h"

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

	[[nodiscard]] int32_t GetCastingStat() const noexcept;

private:
	Character* m_character = nullptr;
	MonsterManager* m_monsterManager = nullptr;
	CombatLog* m_combatLog = nullptr;
	const Dungeon* m_dungeon = nullptr;
};
