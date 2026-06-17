#include "stdafx.h"
#include "game/data/item_generator.h"
#include "core/json_data_manager.h"
#include "core/data_manager.h"
#include <cstdlib>

//=============================================================================

Item ItemGenerator::GenerateWeapon(int32_t playerLevel)
{
	auto& jdm = JsonDataManager::Instance();
	const json& weaponList = jdm.AllWeaponTypes();
	if (weaponList.empty())
	{
		Item fallback;
		fallback.name = "Stick";
		fallback.type = ItemType::Weapon;
		fallback.slot = EquipmentSlot::Weapon;
		fallback.damageMin = 1;
		fallback.damageMax = 4;
		return fallback;
	}

	int32_t idx = std::rand() % static_cast<int32_t>(weaponList.size());
	const json& weapon = weaponList[idx];
	std::string id = weapon.value("id", "sword");

	return generateFromBase(id, weapon, playerLevel);
}

//=============================================================================

Item ItemGenerator::GenerateArmorItem(int32_t playerLevel)
{
	auto& jdm = JsonDataManager::Instance();
	const json& armorList = jdm.AllArmorTypes();
	if (armorList.empty())
	{
		Item fallback;
		fallback.name = "Rags";
		fallback.type = ItemType::Armor;
		fallback.slot = EquipmentSlot::Body;
		fallback.ac = 1;
		return fallback;
	}

	int32_t idx = std::rand() % static_cast<int32_t>(armorList.size());
	const json& armor = armorList[idx];
	std::string id = armor.value("id", "cloth");

	return generateFromBase(id, armor, playerLevel);
}

//=============================================================================

Item ItemGenerator::GenerateAccessory(int32_t playerLevel)
{
	Item item;
	item.name = "Ring";
	item.type = ItemType::Accessory;
	item.slot = EquipmentSlot::Ring;
	item.value = 5 + playerLevel * 2;
	item.atkBonus = 1 + playerLevel / 5;
	item.rarity = ItemRarity::Common;
	return item;
}

//=============================================================================

Item ItemGenerator::GenerateEquipment(int32_t playerLevel)
{
	// 50% weapon, 40% armor, 10% accessory
	int32_t roll = std::rand() % 100;
	if (roll < 50)
	{
		return GenerateWeapon(playerLevel);
	}
	else if (roll < 90)
	{
		return GenerateArmorItem(playerLevel);
	}
	else
	{
		return GenerateAccessory(playerLevel);
	}
}

//=============================================================================

Item ItemGenerator::generateFromBase(
	const std::string& baseItemId,
	const json& baseData,
	int32_t playerLevel)
{
	int32_t tierCap = 1 + playerLevel / 3;
	if (tierCap < 1) { tierCap = 1; }
	if (tierCap > 5) { tierCap = 5; }

	std::string prefixId = pickPrefix(tierCap);
	std::string materialId = pickMaterial(tierCap);
	std::string postfixId = pickPostfix(tierCap);

	auto& jdm = JsonDataManager::Instance();
	const json& prefixData = jdm.GetPrefix(prefixId);
	const json& materialData = jdm.GetMaterial(materialId);
	const json& postfixData = jdm.GetPostfix(postfixId);

	// Determine type
	std::string slotStr = baseData.value("slot", "");
	ItemType type = ItemType::Weapon;
	EquipmentSlot slot = EquipmentSlot::Weapon;
	if (slotStr == "shield")
	{
		type = ItemType::Shield;
		slot = EquipmentSlot::Shield;
	}
	else if (slotStr == "head")
	{
		type = ItemType::Armor;
		slot = EquipmentSlot::Head;
	}
	else if (slotStr == "body")
	{
		type = ItemType::Armor;
		slot = EquipmentSlot::Body;
	}
	else if (slotStr == "hands")
	{
		type = ItemType::Armor;
		slot = EquipmentSlot::Hands;
	}
	else if (slotStr == "feet")
	{
		type = ItemType::Armor;
		slot = EquipmentSlot::Feet;
	}
	else
	{
		type = ItemType::Weapon;
		slot = EquipmentSlot::Weapon;
	}

	ItemStats stats = DataManager::CalculateItemStats(baseData, materialData, prefixData, postfixData);

	// Build name: "Prefix Material BaseName of Postfix"
	std::string fullName;
	std::string prefixName = prefixData.value("name", "");
	std::string materialName = materialData.value("name", "");
	std::string baseName = baseData.value("name", baseItemId);
	std::string postfixName = postfixData.value("name", "");

	if (!prefixName.empty())
	{
		fullName += prefixName + " ";
	}
	if (!materialName.empty() && materialName != baseName)
	{
		fullName += materialName + " ";
	}
	fullName += baseName;
	if (!postfixName.empty())
	{
		fullName += " " + postfixName;
	}

	// Value calculation
	float valueMult = prefixData.value("valueMult", 1.0f)
		* materialData.value("valueMult", 1.0f)
		* postfixData.value("valueMult", 1.0f);
	int32_t baseValue = baseData.value("baseValue", 10);
	int32_t finalValue = static_cast<int32_t>(static_cast<float>(baseValue) * valueMult);

	Item item;
	item.name = fullName;
	item.type = type;
	item.slot = slot;
	item.rarity = determineRarity(prefixData, materialData, postfixData);
	item.value = finalValue;
	item.bonus = stats.damageMin + stats.damageMax / 2; // keep original bonus convention

	item.damageMin = stats.damageMin;
	item.damageMax = stats.damageMax;
	item.ac = stats.ac;
	item.atkBonus = stats.atkBonus;
	item.strBonus = stats.strBonus;
	item.dexBonus = stats.dexBonus;
	item.conBonus = stats.conBonus;
	item.hpBonus = stats.hpBonus;
	item.mpBonus = stats.mpBonus;

	// Element damage (from prefix or postfix)
	if (!prefixData.is_null() && prefixData.contains("elementDamage"))
	{
		const json& elem = prefixData["elementDamage"];
		item.elementType = elem.value("type", "");
		item.elementDamageMin = elem.value("min", 0);
		item.elementDamageMax = elem.value("max", 0);
	}
	if (item.elementType.empty() && !postfixData.is_null() && postfixData.contains("elementDamage"))
	{
		const json& elem = postfixData["elementDamage"];
		item.elementType = elem.value("type", "");
		item.elementDamageMin = elem.value("min", 0);
		item.elementDamageMax = elem.value("max", 0);
	}

	// Life steal (from postfix)
	if (!postfixData.is_null())
	{
		item.lifeStealPercent = postfixData.value("lifeStealPercent", 0);
	}

	return item;
}

//=============================================================================

std::string ItemGenerator::pickPrefix(int32_t tierCap)
{
	auto& jdm = JsonDataManager::Instance();
	const json& list = jdm.AllPrefixes();
	if (list.empty()) { return "plain"; }

	// Collect eligible prefixes (tier <= tierCap)
	std::vector<size_t> candidates;
	for (size_t i = 0; i < list.size(); ++i)
	{
		int32_t t = list[i].value("tier", 0);
		if (t <= tierCap)
		{
			candidates.push_back(i);
		}
	}

	if (candidates.empty()) { return "plain"; }

	size_t chosen = candidates[std::rand() % candidates.size()];
	return list[chosen].value("id", "plain");
}

//=============================================================================

std::string ItemGenerator::pickMaterial(int32_t tierCap)
{
	auto& jdm = JsonDataManager::Instance();
	const json& list = jdm.AllMaterials();
	if (list.empty()) { return "iron"; }

	std::vector<size_t> candidates;
	for (size_t i = 0; i < list.size(); ++i)
	{
		int32_t t = list[i].value("tier", 0);
		if (t <= tierCap)
		{
			candidates.push_back(i);
		}
	}

	if (candidates.empty()) { return "iron"; }

	size_t chosen = candidates[std::rand() % candidates.size()];
	return list[chosen].value("id", "iron");
}

//=============================================================================

std::string ItemGenerator::pickPostfix(int32_t tierCap)
{
	auto& jdm = JsonDataManager::Instance();
	const json& list = jdm.AllPostfixes();
	if (list.empty()) { return "none"; }

	// Weight: "none" (tier 0) always available, plus any other <= tierCap
	std::vector<size_t> candidates;
	for (size_t i = 0; i < list.size(); ++i)
	{
		int32_t t = list[i].value("tier", 0);
		if (t <= tierCap)
		{
			candidates.push_back(i);
		}
	}

	if (candidates.empty()) { return "none"; }

	size_t chosen = candidates[std::rand() % candidates.size()];
	return list[chosen].value("id", "none");
}

//=============================================================================

ItemRarity ItemGenerator::determineRarity(
	const json& prefix, const json& material, const json& postfix)
{
	int32_t maxTier = 0;

	auto getTier = [](const json& j) -> int32_t
	{
		if (j.is_null() || j.empty()) { return 0; }
		return j.value("tier", 0);
	};

	maxTier = std::max(getTier(prefix), getTier(material));
	maxTier = std::max(maxTier, getTier(postfix));

	if (maxTier >= 5) { return ItemRarity::Legendary; }
	if (maxTier >= 3) { return ItemRarity::Rare; }
	if (maxTier >= 2) { return ItemRarity::Uncommon; }
	return ItemRarity::Common;
}
