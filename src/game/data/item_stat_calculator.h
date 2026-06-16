#pragma once

#include <cstdint>
#include "game/combat/item.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct ItemStats final
{
	int32_t damageMin = 0;
	int32_t damageMax = 0;
	int32_t ac = 0;
	int32_t atkBonus = 0;
	int32_t strBonus = 0;
	int32_t dexBonus = 0;
	int32_t conBonus = 0;
	int32_t hpBonus = 0;
	int32_t mpBonus = 0;
};

struct ItemStatCalculator final
{
	[[nodiscard]] static ItemStats Calculate(
		const json& baseItem,
		const json& material,
		const json& prefix,
		const json& postfix
	);
};
