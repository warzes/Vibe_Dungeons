#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "core/json_alias.h"

enum class EffectType : uint8_t
{
	DamageOverTime,  // fire, poison
	Slow,            // ice - makes turns consume 2x time
	Stun,            // skips turn
	Blind,           // -4 atkBonus
	Regen,           // heal over time
	Shield,          // +AC for duration
	Invisibility,    // monsters ignore
	Silence          // cannot cast spells
};

struct StatusEffectDef final
{
	std::string id;         // "burn", "poison", "slow", "stun", etc.
	std::string name;       // "Burn", "Poisoned", etc.
	EffectType type = EffectType::DamageOverTime;
	std::string element;    // "fire", "ice", "poison", "holy", etc.
	int32_t duration = 0;           // in turns
	int32_t value = 0;              // damage/tick or stat mod
	int32_t tickInterval = 1;       // how many turns between ticks
	bool beneficial = false;

	static StatusEffectDef FromJson(const json& j);
};

struct ActiveEffect final
{
	std::string defId;       // references StatusEffectDef::id
	std::string sourceName;  // "Fireball", "Poison Trap", etc.
	int32_t remainingTurns = 0;
	int32_t ticksUntilNext = 1;  // countdown to next tick

	[[nodiscard]] bool IsExpired() const noexcept { return remainingTurns <= 0; }
};

class StatusSystem final
{
public:
	StatusSystem();

	void LoadEffectDefs();

	[[nodiscard]] const StatusEffectDef* GetEffectDef(const std::string& id) const;

	// Apply an effect to a target (checks resistance/immunity)
	void ApplyEffect(
		std::vector<ActiveEffect>& targetEffects,
		const std::string& effectDefId,
		const std::string& sourceName,
		int32_t durationOverride = -1,
		const std::vector<std::string>& resistances = {},
		const std::vector<std::string>& immunities = {}
	);

	// Update all active effects: tick damage, decrement timers, remove expired
	// Returns the total damage/healing this tick
	void ProcessTick(
		std::vector<ActiveEffect>& effects,
		std::vector<std::string>& expiredOut,
		std::function<void(int32_t damage, const std::string& effectName)> onTickDamage
	);

	// Decrement all durations by 1
	void AdvanceTurns(std::vector<ActiveEffect>& effects);

	// Remove all effects of a given type (for Cleanse)
	void RemoveType(std::vector<ActiveEffect>& effects, EffectType type);
	void RemoveAllNegative(std::vector<ActiveEffect>& effects);

	// Check if a specific effect type is active
	[[nodiscard]] static bool HasEffectType(const std::vector<ActiveEffect>& effects, EffectType type);
	[[nodiscard]] static bool HasEffect(const std::vector<ActiveEffect>& effects, const std::string& defId);

	// Calculate stat modifiers from active effects
	[[nodiscard]] int32_t GetAtkBonusMod(const std::vector<ActiveEffect>& effects);
	[[nodiscard]] int32_t GetAcBonusMod(const std::vector<ActiveEffect>& effects);

private:
	std::unordered_map<std::string, StatusEffectDef> m_effectDefs;
};
