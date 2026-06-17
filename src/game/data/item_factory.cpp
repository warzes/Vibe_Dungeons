#include "stdafx.h"
#include "game/data/item_factory.h"
#include "core/json_data_manager.h"

Item ItemFactory::CreateBase(const std::string& itemId)
{
	const json& data = JsonDataManager::Instance().GetItemBaseData(itemId);

	Item item;
	item.name = data.value("name", itemId);
	item.value = data.value("value", 0);

	std::string typeStr = data.value("type", "gold");
	if (typeStr == "potion")
	{
		std::string sub = data.value("subtype", "heal");
		if (sub == "heal")       { item.type = ItemType::PotionHeal; }
		else if (sub == "mana")  { item.type = ItemType::PotionMana; }
	}
	else if (typeStr == "weapon")    { item.type = ItemType::Weapon; }
	else if (typeStr == "armor")     { item.type = ItemType::Armor; }
	else if (typeStr == "shield")    { item.type = ItemType::Shield; }
	else if (typeStr == "key")       { item.type = ItemType::Key; }
	else if (typeStr == "gold")      { item.type = ItemType::Gold; }
	else if (typeStr == "scroll")    { item.type = ItemType::Scroll; }
	else                             { item.type = ItemType::QuestItem; }

	item.bonus = data.value("bonus", 0);

	item.damageMin = data.value("damageMin", 0);
	item.damageMax = data.value("damageMax", 0);
	item.ac = data.value("ac", 0);
	item.atkBonus = data.value("atkBonus", 0);
	item.hpBonus = data.value("hpBonus", 0);
	item.mpBonus = data.value("mpBonus", 0);

	// Map slot string to EquipmentSlot
	std::string slotStr = data.value("slot", "");
	if (slotStr == "weapon")       { item.slot = EquipmentSlot::Weapon; }
	else if (slotStr == "shield")  { item.slot = EquipmentSlot::Shield; }
	else if (slotStr == "head")    { item.slot = EquipmentSlot::Head; }
	else if (slotStr == "body")    { item.slot = EquipmentSlot::Body; }
	else if (slotStr == "hands")   { item.slot = EquipmentSlot::Hands; }
	else if (slotStr == "feet")    { item.slot = EquipmentSlot::Feet; }
	else if (slotStr == "ring")    { item.slot = EquipmentSlot::Ring; }
	else if (slotStr == "amulet")  { item.slot = EquipmentSlot::Amulet; }

	return item;
}

Item ItemFactory::CreatePotion(const std::string& subtype, int32_t value)
{
	Item item;
	if (subtype == "heal")
	{
		item.name = "Healing Potion";
		item.type = ItemType::PotionHeal;
		item.value = (value > 0) ? value : 10;
		item.bonus = item.value;
	}
	else if (subtype == "mana")
	{
		item.name = "Mana Potion";
		item.type = ItemType::PotionMana;
		item.value = (value > 0) ? value : 8;
		item.bonus = item.value;
	}
	return item;
}

Item ItemFactory::CreateGold(int32_t amount)
{
	Item item;
	item.name = "Gold Coins";
	item.type = ItemType::Gold;
	item.value = amount;
	return item;
}

Item ItemFactory::CreateScroll(const std::string& spellId)
{
	const json& spellData = JsonDataManager::Instance().GetSpellData(spellId);

	Item item;
	item.name = "Scroll of " + spellData.value("name", spellId);
	item.type = ItemType::Scroll;
	item.value = spellData.value("mpCost", 5) * 5;
	return item;
}
