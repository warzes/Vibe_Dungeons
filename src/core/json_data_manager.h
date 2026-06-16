#pragma once

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

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

private:
	JsonDataManager() = default;
	~JsonDataManager() = default;
	JsonDataManager(const JsonDataManager&) = delete;
	JsonDataManager& operator=(const JsonDataManager&) = delete;

	bool loadFile(const char* path, json& out, const char* rootKey);

	json m_classes = json::array();
	json m_monsters = json::array();
	json m_itemsBase = json::array();
	json m_weaponTypes = json::array();
	json m_armorTypes = json::array();
	json m_materials = json::array();
	json m_prefixes = json::array();
	json m_postfixes = json::array();
	json m_spells = json::array();
	json m_abilities = json::array();
};
