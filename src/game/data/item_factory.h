#pragma once

#include <string>
#include "game/combat/item.h"

struct ItemFactory final
{
	[[nodiscard]] static Item CreateBase(const std::string& itemId);
	[[nodiscard]] static Item CreatePotion(const std::string& subtype, int32_t value = 0);
	[[nodiscard]] static Item CreateGold(int32_t amount);
	[[nodiscard]] static Item CreateScroll(const std::string& spellId);
};
