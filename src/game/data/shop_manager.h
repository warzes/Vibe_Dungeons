#pragma once

#include <string>
#include <vector>
#include "core/json_alias.h"

struct ShopEntry final
{
	std::string itemId;
	int32_t weight = 1;
	int32_t minCount = 1;
	int32_t maxCount = 1;
};

struct ShopTable final
{
	std::string id;
	std::vector<ShopEntry> entries;
	int32_t maxItems = 8; // how many different items are available at once
};

struct ShopManager final
{
	static void LoadShops(const char* filePath);
	static void Clear() noexcept;

	[[nodiscard]] static const ShopTable* GetShop(const std::string& id);
	[[nodiscard]] static const std::vector<ShopTable>& AllShops() noexcept { return s_shops; }

	// Generate a random shop inventory from a table
	[[nodiscard]] static std::vector<std::string> GenerateInventory(
		const std::string& tableId, int32_t seed);

private:
	static std::vector<ShopTable> s_shops;
};
