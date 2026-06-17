#pragma once

#include "core/json_alias.h"
#include "game/combat/item.h"

struct ItemGenerator final
{
	[[nodiscard]] static Item GenerateWeapon(int32_t playerLevel);
	[[nodiscard]] static Item GenerateArmorItem(int32_t playerLevel);
	[[nodiscard]] static Item GenerateAccessory(int32_t playerLevel);
	[[nodiscard]] static Item GenerateEquipment(int32_t playerLevel);

private:
	[[nodiscard]] static Item generateFromBase(
		const std::string& baseItemId,
		const json& baseData,
		int32_t playerLevel);

	[[nodiscard]] static std::string pickPrefix(int32_t tierCap);
	[[nodiscard]] static std::string pickMaterial(int32_t tierCap);
	[[nodiscard]] static std::string pickPostfix(int32_t tierCap);

	[[nodiscard]] static ItemRarity determineRarity(
		const json& prefix, const json& material, const json& postfix);
};
