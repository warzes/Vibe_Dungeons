#pragma once

#include "game/crafting/crafting_recipe.h"
#include "game/combat/inventory.h"
#include <string>
#include <vector>
#include <cstdint>

struct CraftingCategory final
{
	std::string id;
	std::string name;
	std::string description;
};

class CraftingSystem final
{
public:
	void LoadResources();
	void LoadRecipes();
	void LoadCategories();

	[[nodiscard]] const std::vector<CraftingRecipe>& AllRecipes() const noexcept
	{
		return m_recipes;
	}

	[[nodiscard]] std::vector<const CraftingRecipe*> GetRecipesByCategory(
		const std::string& category) const noexcept;

	[[nodiscard]] std::vector<const CraftingRecipe*> GetAvailableRecipes(
		const Inventory& inventory, int32_t craftingLevel) const noexcept;

	[[nodiscard]] bool CanCraft(
		const CraftingRecipe& recipe,
		const Inventory& inventory) const noexcept;

	// Returns true if crafting succeeded (ingredients consumed, result produced)
	bool Craft(const CraftingRecipe& recipe, Inventory& inventory) noexcept;

	[[nodiscard]] const std::vector<CraftingCategory>& Categories() const noexcept
	{
		return m_categories;
	}

	// Recipe discovery (step 173)
	void UnlockRecipe(const std::string& recipeId) noexcept;
	void UnlockAllRecipes() noexcept;
	[[nodiscard]] bool IsRecipeUnlocked(const std::string& recipeId) const noexcept;

	// Crafting level (steps 173, 182)
	int32_t m_craftingLevel = 1;
	int32_t m_smithingXp = 0;

	// ---- Armorsmith operations (steps 183-190) ----
	int32_t m_armorsmithXp = 0;

	// Step 183-184: Create armor from baseType + material + ingot
	bool CreateArmor(
		Inventory& inventory,
		const std::string& baseTypeId,
		const std::string& materialId) noexcept;

	// Step 185: Upgrade armor — add/improve prefix, +AC
	bool UpgradeArmor(Item& item, const std::string& materialId) noexcept;

	// Step 186: Add postfix — armor + essence → of Protection (+AC), of Vitality (+HP)
	bool AddArmorPostfix(Item& item, const std::string& essenceId) noexcept;

	// Step 187: Repair armor — restore durability
	bool RepairArmor(Item& item) noexcept;

	// Step 189: Enchant — armor + scroll + essence → magical effect
	bool EnchantArmor(Item& item, const std::string& scrollId, const std::string& essenceId) noexcept;

	// Step 190: Gain armorsmith XP
	void AddArmorsmithXp(int32_t amount) noexcept;
	int32_t GetArmorsmithLevel() const noexcept;

	// Check if an item is armor (equippable armor piece)
	[[nodiscard]] static bool IsArmor(const Item& item) noexcept;

	// ---- Weaponsmith operations (steps 174-182) ----

	// Step 174: Create weapon from base type + material + ingot
	bool CreateWeapon(
		Inventory& inventory,
		const std::string& baseTypeId,
		const std::string& materialId) noexcept;

	// Step 175: Upgrade weapon — add/improve prefix using material+ingot
	bool UpgradeWeapon(Item& item, const std::string& materialId) noexcept;

	// Step 176: Sharpen weapon — +1 damage (max +3)
	bool SharpenWeapon(Item& item) noexcept;

	// Step 177: Add postfix — weapon + essence → of Fire, etc.
	bool AddPostfix(Item& item, const std::string& essenceId) noexcept;

	// Step 179: Repair weapon — +material → restore durability
	bool RepairItem(Item& item) noexcept;

	// Step 182: Gain smithing XP
	void AddSmithingXp(int32_t amount) noexcept;
	int32_t GetSmithingLevel() const noexcept;

	// Check if an item in inventory is a weapon (slot == Weapon)
	[[nodiscard]] static bool IsWeapon(const Item& item) noexcept;

	// Check for ingredients in inventory
	[[nodiscard]] bool HasIngredients(Inventory& inv, const std::string& itemId, int32_t count) noexcept;

private:
	std::vector<CraftingRecipe> m_recipes;
	std::vector<CraftingCategory> m_categories;
	std::vector<std::string> m_unlockedRecipes;
	bool m_allUnlocked = false;

	[[nodiscard]] CraftingRecipe* findRecipe(const std::string& id) noexcept;
	[[nodiscard]] int32_t countItem(const Inventory& inventory, const std::string& itemId) const noexcept;
	bool removeItems(Inventory& inventory, const std::string& itemId, int32_t count) noexcept;
};
