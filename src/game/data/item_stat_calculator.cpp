#include "stdafx.h"
#include "game/data/item_stat_calculator.h"

ItemStats ItemStatCalculator::Calculate(
	const json& baseItem,
	const json& material,
	const json& prefix,
	const json& postfix)
{
	ItemStats stats;

	// Base values
	stats.damageMin = baseItem.value("damageMin", 0);
	stats.damageMax = baseItem.value("damageMax", 0);
	stats.ac = baseItem.value("ac", 0);

	// Material modifiers
	if (!material.is_null() && !material.empty())
	{
		float dmgMult = material.value("damageMult", 1.0f);
		stats.damageMin = static_cast<int32_t>(static_cast<float>(stats.damageMin) * dmgMult);
		stats.damageMax = static_cast<int32_t>(static_cast<float>(stats.damageMax) * dmgMult);
		stats.ac += material.value("acBonus", 0);
	}

	// Prefix modifiers
	if (!prefix.is_null() && !prefix.empty())
	{
		stats.damageMin += prefix.value("damageBonus", 0);
		stats.damageMax += prefix.value("damageBonus", 0);
		stats.atkBonus += prefix.value("atkBonus", 0);
		stats.ac += prefix.value("acBonus", 0);
	}

	// Postfix modifiers
	if (!postfix.is_null() && !postfix.empty())
	{
		stats.damageMin += postfix.value("damageBonus", 0);
		stats.damageMax += postfix.value("damageBonus", 0);
		stats.atkBonus += postfix.value("atkBonus", 0);
		stats.ac += postfix.value("acBonus", 0);
		stats.strBonus += postfix.value("strBonus", 0);
		stats.dexBonus += postfix.value("dexBonus", 0);
		stats.conBonus += postfix.value("conBonus", 0);
		stats.hpBonus += postfix.value("hpBonus", 0);
	}

	// Ensure minimums
	if (stats.damageMin < 1) { stats.damageMin = 1; }
	if (stats.damageMax < stats.damageMin) { stats.damageMax = stats.damageMin; }

	return stats;
}
