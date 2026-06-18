#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include "core/json_alias.h"

struct CraftingIngredient final
{
	std::string itemId;
	int32_t count = 1;
};

struct CraftingResult final
{
	std::string itemId;
	int32_t count = 1;
};

struct CraftingRecipe final
{
	std::string id;
	std::string name;
	std::string category;
	std::string stationType;
	int32_t skillReq = 1;
	std::vector<CraftingIngredient> ingredients;
	CraftingResult result;
	bool unlocked = true;

	static CraftingRecipe FromJson(const json& j)
	{
		CraftingRecipe r;
		r.id = j.value("id", std::string());
		r.name = j.value("name", std::string());
		r.category = j.value("category", std::string());
		r.stationType = j.value("stationType", std::string());
		r.skillReq = j.value("skillReq", 1);

		if (j.contains("ingredients") && j["ingredients"].is_array())
		{
			for (const auto& ing : j["ingredients"])
			{
				CraftingIngredient ci;
				ci.itemId = ing.value("id", std::string());
				ci.count = ing.value("count", 1);
				r.ingredients.push_back(std::move(ci));
			}
		}

		// Support both "result" and "resultItem" keys
		const json* resJson = nullptr;
		if (j.contains("result") && j["result"].is_object())
		{
			resJson = &j["result"];
		}
		else if (j.contains("resultItem") && j["resultItem"].is_object())
		{
			resJson = &j["resultItem"];
		}

		if (resJson)
		{
			r.result.itemId = resJson->value("id", std::string());
			r.result.count = resJson->value("count", 1);
		}

		return r;
	}
};
