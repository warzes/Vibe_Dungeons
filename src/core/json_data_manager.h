#pragma once

#include "core/json_alias.h"

class JsonDataManager final
{
public:
	static JsonDataManager& Instance();

	bool LoadAll(const char* dataDir);

	[[nodiscard]] const json& GetClassData(const std::string& classId) const;
	[[nodiscard]] const json& GetMonsterData(const std::string& monsterId) const;
	[[nodiscard]] const json& GetItemBaseData(const std::string& itemId) const;
	[[nodiscard]] const json& GetWeaponType(const std::string& id) const;
	[[nodiscard]] const json& GetArmorType(const std::string& id) const;
	[[nodiscard]] const json& GetMaterial(const std::string& id) const;
	[[nodiscard]] const json& GetPrefix(const std::string& id) const;
	[[nodiscard]] const json& GetPostfix(const std::string& id) const;
	[[nodiscard]] const json& GetSpellData(const std::string& spellId) const;
	[[nodiscard]] const json& GetAbilityData(const std::string& abilityId) const;
	[[nodiscard]] const json& GetLevelTable() const noexcept { return m_levelTable; }

	[[nodiscard]] const json& AllClasses() const noexcept { return m_classes; }
	[[nodiscard]] const json& AllMonsters() const noexcept { return m_monsters; }
	[[nodiscard]] const json& AllItemBases() const noexcept { return m_itemsBase; }
	[[nodiscard]] const json& AllWeaponTypes() const noexcept { return m_weaponTypes; }
	[[nodiscard]] const json& AllArmorTypes() const noexcept { return m_armorTypes; }
	[[nodiscard]] const json& AllMaterials() const noexcept { return m_materials; }
	[[nodiscard]] const json& AllPrefixes() const noexcept { return m_prefixes; }
	[[nodiscard]] const json& AllPostfixes() const noexcept { return m_postfixes; }
	[[nodiscard]] const json& AllSpells() const noexcept { return m_spells; }
	[[nodiscard]] const json& AllAbilities() const noexcept { return m_abilities; }
	[[nodiscard]] const json& AllStatusEffects() const noexcept { return m_statusEffects; }
	[[nodiscard]] const json& AllEncounters() const noexcept { return m_encounters; }
	[[nodiscard]] const json& GetResourceData(const std::string& resourceId) const;
	[[nodiscard]] const json& GetRecipeData(const std::string& recipeId) const;
	[[nodiscard]] const json& AllResources() const noexcept { return m_resources; }
	[[nodiscard]] const json& AllRecipes() const noexcept { return m_recipes; }
	[[nodiscard]] const json& AllCategories() const noexcept { return m_categories; }

private:
	JsonDataManager() = default;
	~JsonDataManager() = default;
	JsonDataManager(const JsonDataManager&) = delete;
	JsonDataManager& operator=(const JsonDataManager&) = delete;

	bool loadFile(const char* path, json& out, const char* rootKey);

	void buildIndex(const json& array, std::unordered_map<std::string, size_t>& out, const char* key) noexcept;

	json m_classes = json::array();
	std::unordered_map<std::string, size_t> m_classIndex;
	json m_monsters = json::array();
	std::unordered_map<std::string, size_t> m_monsterIndex;
	json m_itemsBase = json::array();
	std::unordered_map<std::string, size_t> m_itemBaseIndex;
	json m_weaponTypes = json::array();
	std::unordered_map<std::string, size_t> m_weaponTypeIndex;
	json m_armorTypes = json::array();
	std::unordered_map<std::string, size_t> m_armorTypeIndex;
	json m_materials = json::array();
	std::unordered_map<std::string, size_t> m_materialIndex;
	json m_prefixes = json::array();
	std::unordered_map<std::string, size_t> m_prefixIndex;
	json m_postfixes = json::array();
	std::unordered_map<std::string, size_t> m_postfixIndex;
	json m_spells = json::array();
	std::unordered_map<std::string, size_t> m_spellIndex;
	json m_abilities = json::array();
	std::unordered_map<std::string, size_t> m_abilityIndex;
	json m_statusEffects = json::array();
	json m_encounters = json::array();
	json m_resources = json::array();
	std::unordered_map<std::string, size_t> m_resourceIndex;
	json m_recipes = json::array();
	std::unordered_map<std::string, size_t> m_recipeIndex;
	json m_categories = json::array();
	json m_levelTable = json::array();
};
