#include "stdafx.h"
#include "game/data/shop_manager.h"
#include "core/file_io.h"
#include "core/logger.h"

#include <random>
#include <algorithm>

std::vector<ShopTable> ShopManager::s_shops;

void ShopManager::LoadShops(const char* filePath)
{
	Clear();

	std::string text = FileReadString(filePath);
	if (text.empty())
	{
		Logger::Warn(std::string("ShopManager: could not read ") + filePath);
		return;
	}

	json j = json::parse(text, nullptr, false);
	if (j.is_discarded() || !j.contains("shops"))
	{
		Logger::Warn(std::string("ShopManager: invalid JSON in ") + filePath);
		return;
	}

	for (const auto& entry : j["shops"])
	{
		ShopTable shop;
		shop.id = entry.value("id", "");
		shop.maxItems = entry.value("maxItems", 8);

		if (entry.contains("entries"))
		{
			for (const auto& e : entry["entries"])
			{
				ShopEntry se;
				se.itemId = e.value("itemId", "");
				se.weight = e.value("weight", 1);
				se.minCount = e.value("minCount", 1);
				se.maxCount = e.value("maxCount", 1);
				shop.entries.push_back(std::move(se));
			}
		}

		s_shops.push_back(std::move(shop));
	}

	Logger::Info(std::string("ShopManager: loaded ") + std::to_string(s_shops.size()) + " shops");
}

void ShopManager::Clear() noexcept
{
	s_shops.clear();
}

const ShopTable* ShopManager::GetShop(const std::string& id)
{
	for (const auto& shop : s_shops)
	{
		if (shop.id == id)
		{
			return &shop;
		}
	}
	return nullptr;
}

std::vector<std::string> ShopManager::GenerateInventory(
	const std::string& tableId, int32_t seed)
{
	std::vector<std::string> result;

	const ShopTable* table = GetShop(tableId);
	if (!table || table->entries.empty())
	{
		return result;
	}

	std::mt19937 rng(seed);

	// Build weighted list
	std::vector<std::string> weightedPool;
	for (const auto& entry : table->entries)
	{
		for (int32_t w = 0; w < entry.weight; ++w)
		{
			weightedPool.push_back(entry.itemId);
		}
	}

	if (weightedPool.empty()) return result;

	std::shuffle(weightedPool.begin(), weightedPool.end(), rng);

	int32_t count = std::min(table->maxItems, static_cast<int32_t>(weightedPool.size()));
	for (int32_t i = 0; i < count; ++i)
	{
		result.push_back(weightedPool[i]);
	}

	return result;
}
