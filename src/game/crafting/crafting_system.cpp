#include "stdafx.h"
#include "game/crafting/crafting_system.h"
#include "core/json_data_manager.h"
#include "core/data_manager.h"
#include "core/logger.h"
#include "game/combat/item.h"
#include "game/data/item_factory.h"

//=============================================================================

void CraftingSystem::LoadResources()
{
	// Resources are loaded directly via JsonDataManager;
	// this method exists for future resource-specific logic.
	Logger::Info("CraftingSystem: resources loaded via JsonDataManager");
}

//=============================================================================

void CraftingSystem::LoadRecipes()
{
	m_recipes.clear();
	const json& recipesJson = JsonDataManager::Instance().AllRecipes();
	if (!recipesJson.is_array())
	{
		Logger::Warn("CraftingSystem: no recipes found in data");
		return;
	}

	for (const auto& j : recipesJson)
	{
		m_recipes.push_back(CraftingRecipe::FromJson(j));
	}

	Logger::Info("CraftingSystem: loaded " + std::to_string(m_recipes.size()) + " recipes");
}

//=============================================================================

void CraftingSystem::LoadCategories()
{
	m_categories.clear();
	const json& catsJson = JsonDataManager::Instance().AllCategories();
	if (!catsJson.is_array())
	{
		return;
	}

	for (const auto& j : catsJson)
	{
		CraftingCategory cat;
		cat.id = j.value("id", std::string());
		cat.name = j.value("name", std::string());
		cat.description = j.value("description", std::string());
		m_categories.push_back(std::move(cat));
	}
}

//=============================================================================

std::vector<const CraftingRecipe*> CraftingSystem::GetRecipesByCategory(
	const std::string& category) const noexcept
{
	std::vector<const CraftingRecipe*> result;
	for (const auto& r : m_recipes)
	{
		if (r.category == category)
		{
			result.push_back(&r);
		}
	}
	return result;
}

//=============================================================================

std::vector<const CraftingRecipe*> CraftingSystem::GetAvailableRecipes(
	const Inventory& inventory, int32_t craftingLevel) const noexcept
{
	std::vector<const CraftingRecipe*> result;
	for (auto& r : m_recipes)
	{
		if (r.skillReq > craftingLevel)
		{
			continue;
		}
		if (!m_allUnlocked)
		{
			bool found = false;
			for (const auto& id : m_unlockedRecipes)
			{
				if (id == r.id)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				continue;
			}
		}
		if (CanCraft(r, inventory))
		{
			result.push_back(&r);
		}
	}
	return result;
}

//=============================================================================

bool CraftingSystem::CanCraft(
	const CraftingRecipe& recipe,
	const Inventory& inventory) const noexcept
{
	for (const auto& ing : recipe.ingredients)
	{
		if (countItem(inventory, ing.itemId) < ing.count)
		{
			return false;
		}
	}
	return true;
}

//=============================================================================

bool CraftingSystem::Craft(const CraftingRecipe& recipe, Inventory& inventory) noexcept
{
	if (!CanCraft(recipe, inventory))
	{
		return false;
	}

	// Consume ingredients
	for (const auto& ing : recipe.ingredients)
	{
		if (!removeItems(inventory, ing.itemId, ing.count))
		{
			return false;
		}
	}

	// Create result via ItemFactory
	Item resultItem = ItemFactory::CreateBase(recipe.result.itemId);

	// Apply count
	for (int32_t i = 0; i < recipe.result.count; ++i)
	{
		inventory.Add(resultItem);
	}

	return true;
}

//=============================================================================

void CraftingSystem::UnlockRecipe(const std::string& recipeId) noexcept
{
	if (m_allUnlocked)
	{
		return;
	}

	for (const auto& id : m_unlockedRecipes)
	{
		if (id == recipeId)
		{
			return; // already unlocked
		}
	}
	m_unlockedRecipes.push_back(recipeId);
}

//=============================================================================

void CraftingSystem::UnlockAllRecipes() noexcept
{
	m_allUnlocked = true;
	m_unlockedRecipes.clear();
}

//=============================================================================

bool CraftingSystem::IsRecipeUnlocked(const std::string& recipeId) const noexcept
{
	if (m_allUnlocked)
	{
		return true;
	}
	for (const auto& id : m_unlockedRecipes)
	{
		if (id == recipeId)
		{
			return true;
		}
	}
	return false;
}

//=============================================================================

CraftingRecipe* CraftingSystem::findRecipe(const std::string& id) noexcept
{
	for (auto& r : m_recipes)
	{
		if (r.id == id)
		{
			return &r;
		}
	}
	return nullptr;
}

//=============================================================================

int32_t CraftingSystem::countItem(const Inventory& inventory, const std::string& itemId) const noexcept
{
	int32_t total = 0;
	for (size_t i = 0; i < inventory.Size(); ++i)
	{
		const Item* item = inventory.Get(i);
		if (item && item->itemId == itemId)
		{
			++total;
		}
	}
	return total;
}

//=============================================================================

bool CraftingSystem::removeItems(Inventory& inventory, const std::string& itemId, int32_t count) noexcept
{
	// Collect indices of matching items (scan backwards to simplify removal)
	std::vector<size_t> indices;
	for (size_t i = 0; i < inventory.Size(); ++i)
	{
		const Item* item = inventory.Get(i);
		if (item && item->itemId == itemId)
		{
			indices.push_back(i);
			if (static_cast<int32_t>(indices.size()) >= count)
			{
				break;
			}
		}
	}

	if (static_cast<int32_t>(indices.size()) < count)
	{
		return false;
	}

	// Remove from last to first so indices stay valid
	for (int32_t i = static_cast<int32_t>(indices.size()) - 1; i >= 0; --i)
	{
		inventory.Remove(indices[i]);
	}

	return true;
}

//=============================================================================
//  Weaponsmith operations (steps 174-182)
//=============================================================================

bool CraftingSystem::IsWeapon(const Item& item) noexcept
{
	return item.slot == EquipmentSlot::Weapon
		|| item.type == ItemType::Weapon;
}

//=============================================================================

bool CraftingSystem::HasIngredients(Inventory& inv, const std::string& itemId, int32_t count) noexcept
{
	return countItem(inv, itemId) >= count;
}

//=============================================================================

// Step 174: Create weapon from base baseType + material
bool CraftingSystem::CreateWeapon(
	Inventory& inventory,
	const std::string& baseTypeId,
	const std::string& materialId) noexcept
{
	// Consume ingredients: check we have them
	if (!HasIngredients(inventory, "iron_ingot", 1))
	{
		return false;
	}

	// Get base weapon type data
	const json& weaponData = JsonDataManager::Instance().GetWeaponType(baseTypeId);
	if (weaponData.is_null() || weaponData.empty())
	{
		return false;
	}

	const json& materialData = JsonDataManager::Instance().GetMaterial(materialId);
	if (materialData.is_null() || materialData.empty())
	{
		return false;
	}

	// Consume the ingot
	removeItems(inventory, "iron_ingot", 1);

	// Build the weapon using ItemGenerator-like logic
	ItemStats stats = DataManager::CalculateItemStats(weaponData, materialData, json::object(), json::object());

	std::string matName = materialData.value("name", materialId);
	std::string baseName = weaponData.value("name", baseTypeId);

	Item weapon;
	weapon.itemId = "crafted_" + materialId + "_" + baseTypeId;
	weapon.name = matName + " " + baseName;
	weapon.type = ItemType::Weapon;
	weapon.slot = EquipmentSlot::Weapon;
	weapon.materialId = materialId;
	weapon.value = weaponData.value("baseValue", 10) + materialData.value("tier", 1) * 5;
	weapon.damageMin = stats.damageMin;
	weapon.damageMax = stats.damageMax;
	weapon.atkBonus = stats.atkBonus;
	weapon.durability = 100;
	weapon.maxDurability = 100;

	inventory.Add(weapon);
	AddSmithingXp(10 + materialData.value("tier", 1) * 5);

	return true;
}

//=============================================================================

// Step 175: Upgrade weapon — add/improve prefix using material+ingot
bool CraftingSystem::UpgradeWeapon(Item& item, const std::string& materialId) noexcept
{
	if (!IsWeapon(item))
	{
		return false;
	}

	// Determine the new prefix based on material tier
	const json& matData = JsonDataManager::Instance().GetMaterial(materialId);
	if (matData.is_null() || matData.empty())
	{
		return false;
	}

	int32_t matTier = matData.value("tier", 1);
	std::string newPrefix = materialId;

	// If weapon already has a prefix, check if this is better
	if (!item.prefixId.empty())
	{
		const json& oldPrefix = JsonDataManager::Instance().GetPrefix(item.prefixId);
		int32_t oldTier = oldPrefix.value("tier", 0);
		if (matTier <= oldTier)
		{
			return false; // not an upgrade
		}
	}

	item.prefixId = newPrefix;

	// Apply stat bonuses from the material as if it were a prefix
	std::string matName = matData.value("name", materialId);
	item.name = matName + " " + item.name;

	item.damageMin += matTier;
	item.damageMax += matTier;
	item.atkBonus += matTier;

	AddSmithingXp(15 + matTier * 5);

	return true;
}

//=============================================================================

// Step 176: Sharpen weapon — +1 damage (max +3)
bool CraftingSystem::SharpenWeapon(Item& item) noexcept
{
	if (!IsWeapon(item))
	{
		return false;
	}

	if (item.sharpnessLevel >= 3)
	{
		return false; // already max sharpness
	}

	item.sharpnessLevel++;
	item.damageMin += 1;
	item.damageMax += 1;

	item.name += " (sharpened +" + std::to_string(item.sharpnessLevel) + ")";

	AddSmithingXp(20);
	return true;
}

//=============================================================================

// Step 177: Add postfix — weapon + essence → of Fire, etc.
bool CraftingSystem::AddPostfix(Item& item, const std::string& essenceId) noexcept
{
	if (!IsWeapon(item))
	{
		return false;
	}

	// Map essence ID to postfix
	std::string postfixName;
	int32_t elementDmg = 0;
	std::string elementType;

	if (essenceId == "essence_fire" || essenceId == "fire_essence")
	{
		postfixName = "of Fire";
		elementType = "fire";
		elementDmg = 3;
	}
	else if (essenceId == "essence_ice" || essenceId == "ice_essence")
	{
		postfixName = "of Ice";
		elementType = "ice";
		elementDmg = 3;
	}
	else if (essenceId == "essence_poison" || essenceId == "poison_essence")
	{
		postfixName = "of Poison";
		elementType = "poison";
		elementDmg = 2;
	}
	else
	{
		return false;
	}

	// Remove old postfix if any (reverse the name change)
	if (!item.postfixId.empty())
	{
		// Remove old postfix from name
		auto pos = item.name.rfind(" ");
		if (pos != std::string::npos)
		{
			item.name = item.name.substr(0, pos);
		}
	}

	item.postfixId = essenceId;
	item.name += " " + postfixName;
	item.elementType = elementType;
	item.elementDamageMin = elementDmg;
	item.elementDamageMax = elementDmg + 2;

	AddSmithingXp(25);
	return true;
}

//=============================================================================

// Step 179: Repair weapon — restore durability
bool CraftingSystem::RepairItem(Item& item) noexcept
{
	if (item.durability >= item.maxDurability)
	{
		return false; // already full
	}

	item.durability = item.maxDurability;

	AddSmithingXp(5);
	return true;
}

//=============================================================================

// Step 182: Gain smithing XP
void CraftingSystem::AddSmithingXp(int32_t amount) noexcept
{
	m_smithingXp += amount;

	// Level up every 100 XP
	int32_t newLevel = 1 + m_smithingXp / 100;
	if (newLevel > m_craftingLevel)
	{
		m_craftingLevel = newLevel;
	}
}

//=============================================================================

int32_t CraftingSystem::GetSmithingLevel() const noexcept
{
	return m_craftingLevel;
}
