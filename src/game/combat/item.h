#pragma once

#include "game/grid_position.h"

enum class ItemType : uint8_t
{
	Weapon,
	Armor,
	Shield,
	PotionHeal,
	PotionMana,
	Key,
	Gold,
	Scroll,
	QuestItem
};

enum class ItemRarity : uint8_t
{
	Common,
	Uncommon,
	Rare,
	Legendary
};

struct Item final
{
	std::string name;
	ItemType type = ItemType::Gold;
	ItemRarity rarity = ItemRarity::Common;
	int32_t value = 0;
	int32_t bonus = 0;

	[[nodiscard]] const char* SpritePath() const noexcept
	{
		switch (type)
		{
			case ItemType::PotionHeal:  return "data/item_potion_heal.png";
			case ItemType::PotionMana:  return "data/item_potion_mana.png";
			case ItemType::Key:         return "data/item_key.png";
			case ItemType::Gold:        return "data/item_gold.png";
			case ItemType::Scroll:      return "data/item_scroll.png";
			case ItemType::Weapon:      return "data/item_sword.png";
			case ItemType::Armor:       return "data/item_armor.png";
			case ItemType::Shield:      return "data/item_shield.png";
			case ItemType::QuestItem:   return "data/item_quest.png";
		}
		return "data/item_default.png";
	}
};

struct ItemDrop final
{
	Item item;
	GridPosition position;
};
