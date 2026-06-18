#pragma once

#include "game/grid_position.h"
#include "game/direction.h"
#include "game/combat/inventory.h"
#include "game/combat/equipment.h"
#include "game/data/skill_manager.h"
#include "game/combat/status_effect.h"

class Character final
{
public:
	static constexpr int32_t NUM_ACTION_SLOTS = 9;

	// ---- Stats accessors ----
	[[nodiscard]] const std::string& GetName() const noexcept { return m_name; }
	void SetName(std::string_view v) { m_name = v; }

	[[nodiscard]] const std::string& GetClass() const noexcept { return m_charClass; }
	void SetClass(std::string_view v) { m_charClass = v; }

	[[nodiscard]] int32_t GetLevel() const noexcept { return m_level; }
	void SetLevel(int32_t v) noexcept { m_level = v; }

	[[nodiscard]] int32_t GetHp() const noexcept { return m_hp; }
	[[nodiscard]] int32_t GetMaxHp() const noexcept { return m_maxHp; }
	void TakeDamage(int32_t amount) noexcept;
	void Heal(int32_t amount) noexcept;
	void SetHp(int32_t v) noexcept { m_hp = v; }
	void SetMaxHp(int32_t v) noexcept { m_maxHp = v; }

	[[nodiscard]] int32_t GetMp() const noexcept { return m_mp; }
	[[nodiscard]] int32_t GetMaxMp() const noexcept
	{
		int32_t base = 5 + m_intel * 2 + m_level * 2;
		if (m_charClass == "barbarian")  { base = std::max(base, 0); }
		if (m_charClass == "mage")       { base += m_level * 3; }
		if (m_charClass == "war_priest") { base += m_level * 2; }
		if (m_charClass == "paladin")    { base += m_level; }
		return std::max(base + m_equipment.GetTotalStats().mpBonus, 0);
	}
	[[nodiscard]] int32_t GetMpRegenPerTurn() const noexcept
	{
		if (m_charClass == "mage")       { return 2; }
		if (m_charClass == "war_priest") { return 1; }
		return 0;
	}
	void SpendMp(int32_t amount) noexcept;
	void RestoreMp(int32_t amount) noexcept;
	void SetMp(int32_t v) noexcept { m_mp = v; }
	void SetMaxMp(int32_t v) noexcept { m_maxMp = v; }

	[[nodiscard]] int32_t GetAc() const noexcept { return m_ac; }
	[[nodiscard]] int32_t GetEquippedAc() const noexcept { return m_ac + m_equipment.GetTotalStats().ac; }
	void SetAc(int32_t v) noexcept { m_ac = v; }

	[[nodiscard]] int32_t GetStr() const noexcept { return m_str; }
	void SetStr(int32_t v) noexcept { m_str = v; }

	[[nodiscard]] int32_t GetDex() const noexcept { return m_dex; }
	void SetDex(int32_t v) noexcept { m_dex = v; }

	[[nodiscard]] int32_t GetCon() const noexcept { return m_con; }
	void SetCon(int32_t v) noexcept { m_con = v; }

	[[nodiscard]] int32_t GetIntel() const noexcept { return m_intel; }
	void SetIntel(int32_t v) noexcept { m_intel = v; }

	[[nodiscard]] int32_t GetAtkBonus() const noexcept { return m_atkBonus; }
	[[nodiscard]] int32_t GetEquippedAtkBonus() const noexcept { return m_atkBonus + m_equipment.GetTotalStats().atkBonus; }
	void SetAtkBonus(int32_t v) noexcept { m_atkBonus = v; }

	[[nodiscard]] int32_t GetDamageBonus() const noexcept { return m_damageBonus; }
	void SetDamageBonus(int32_t v) noexcept { m_damageBonus = v; }

	[[nodiscard]] int32_t GetDamageMin() const noexcept { return m_damageMin; }
	[[nodiscard]] int32_t GetEquippedDamageMin() const noexcept { return m_damageMin + m_equipment.GetTotalStats().damageMin; }
	void SetDamageMin(int32_t v) noexcept { m_damageMin = v; }

	[[nodiscard]] int32_t GetDamageMax() const noexcept { return m_damageMax; }
	[[nodiscard]] int32_t GetEquippedDamageMax() const noexcept { return m_damageMax + m_equipment.GetTotalStats().damageMax; }
	void SetDamageMax(int32_t v) noexcept { m_damageMax = v; }

	[[nodiscard]] int32_t GetXp() const noexcept { return m_xp; }
	void AddXp(int32_t amount) noexcept { m_xp += amount; }
	void SetXp(int32_t v) noexcept { m_xp = v; }

	[[nodiscard]] int32_t GetXpForNext() const noexcept { return m_xpForNext; }
	void SetXpForNext(int32_t v) noexcept { m_xpForNext = v; }

	// ---- Position ----
	[[nodiscard]] const GridPosition& GetPosition() const noexcept { return m_position; }
	[[nodiscard]] GridPosition& GetPosition() noexcept { return m_position; }

	[[nodiscard]] Direction GetFacing() const noexcept { return m_facing; }
	void SetFacing(Direction v) noexcept { m_facing = v; }

	// ---- Inventory ----
	[[nodiscard]] Inventory& GetInventory() noexcept { return m_inventory; }
	[[nodiscard]] const Inventory& GetInventory() const noexcept { return m_inventory; }

	// ---- Equipment ----
	[[nodiscard]] Equipment& GetEquipment() noexcept { return m_equipment; }
	[[nodiscard]] const Equipment& GetEquipment() const noexcept { return m_equipment; }

	// ---- Skills ----
	[[nodiscard]] std::vector<std::string>& GetUnlockedSkills() noexcept { return m_unlockedSkills; }
	[[nodiscard]] const std::vector<std::string>& GetUnlockedSkills() const noexcept { return m_unlockedSkills; }

	[[nodiscard]] std::array<ActionSlot, NUM_ACTION_SLOTS>& GetActionSlots() noexcept { return m_actionSlots; }
	[[nodiscard]] const std::array<ActionSlot, NUM_ACTION_SLOTS>& GetActionSlots() const noexcept { return m_actionSlots; }

	[[nodiscard]] std::vector<std::string>& GetLearnedSpells() noexcept { return m_learnedSpells; }
	[[nodiscard]] const std::vector<std::string>& GetLearnedSpells() const noexcept { return m_learnedSpells; }

	// ---- Status effects (steps 149-150) ----
	[[nodiscard]] std::vector<ActiveEffect>& GetActiveEffects() noexcept { return m_activeEffects; }
	[[nodiscard]] const std::vector<ActiveEffect>& GetActiveEffects() const noexcept { return m_activeEffects; }

	// ---- Hunger system (step 211) ----
	[[nodiscard]] int32_t GetHunger() const noexcept { return m_hunger; }
	void SetHunger(int32_t v) noexcept { m_hunger = std::clamp(v, 0, MAX_HUNGER); }
	void AddHunger(int32_t amount) noexcept { SetHunger(m_hunger + amount); }
	void ConsumeHunger(int32_t amount) noexcept { SetHunger(m_hunger - amount); }
	static constexpr int32_t MAX_HUNGER = 100;
	static constexpr int32_t HUNGER_WARNING = 30;
	static constexpr int32_t HUNGER_STARVING = 15;

	// ---- Buffs from food (step 222) ----
	struct FoodBuff final
	{
		std::string id;
		std::string name;
		int32_t atkBonus = 0;
		int32_t acBonus = 0;
		int32_t hpRegen = 0;
		int32_t mpRegen = 0;
		int32_t remainingTurns = 0;
	};
	[[nodiscard]] std::vector<FoodBuff>& GetActiveBuffs() noexcept { return m_activeBuffs; }
	[[nodiscard]] const std::vector<FoodBuff>& GetActiveBuffs() const noexcept { return m_activeBuffs; }

	// ---- Serialization ----
	friend void to_json(json& j, const Character& c);
	friend void from_json(const json& j, Character& c);

private:
	std::string m_name = "Hero";
	std::string m_charClass = "barbarian";
	int32_t m_level = 1;
	int32_t m_hp = 24;
	int32_t m_maxHp = 24;
	int32_t m_mp = 0;
	int32_t m_maxMp = 0;
	int32_t m_ac = 12;
	int32_t m_str = 16;
	int32_t m_dex = 10;
	int32_t m_con = 14;
	int32_t m_intel = 8;
	int32_t m_atkBonus = 3;
	int32_t m_damageBonus = 2;
	int32_t m_damageMin = 1;
	int32_t m_damageMax = 6;
	int32_t m_xp = 0;
	int32_t m_xpForNext = 100;
	GridPosition m_position;
	Direction m_facing = Direction::North;
	Inventory m_inventory;
	Equipment m_equipment;
	std::vector<std::string> m_unlockedSkills;
	std::array<ActionSlot, NUM_ACTION_SLOTS> m_actionSlots{};
	std::vector<std::string> m_learnedSpells;
	std::vector<ActiveEffect> m_activeEffects;
	int32_t m_hunger = 80;
	std::vector<FoodBuff> m_activeBuffs;
};
