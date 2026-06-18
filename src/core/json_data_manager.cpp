#include "stdafx.h"
#include "core/json_data_manager.h"
#include "core/file_io.h"
#include "core/logger.h"
#include "core/exception.h"

JsonDataManager& JsonDataManager::Instance()
{
	static JsonDataManager instance;
	return instance;
}

bool JsonDataManager::LoadAll(const char* dataDir)
{
	struct FileEntry
	{
		const char* filename;
		const char* rootKey;
		json* target;
	};

	FileEntry files[] =
	{
		{"classes.json",      "classes",     &m_classes},
		{"monsters.json",     "monsters",    &m_monsters},
		{"items_base.json",   "items",       &m_itemsBase},
		{"weapon_types.json", "weaponTypes", &m_weaponTypes},
		{"armor_types.json",  "armorTypes",  &m_armorTypes},
		{"materials.json",    "materials",   &m_materials},
		{"prefixes.json",     "prefixes",    &m_prefixes},
		{"postfixes.json",    "postfixes",   &m_postfixes},
		{"spells.json",       "spells",      &m_spells},
		{"abilities.json",    "abilities",   &m_abilities},
		{"level_table.json",  "levels",      &m_levelTable}
	};

	// Optional new data files (not required for game to run)
	FileEntry optionalFiles[] =
	{
		{"status_effects.json", "statusEffects", &m_statusEffects},
		{"encounter_groups.json", "encounters", &m_encounters}
	};

	bool allOk = true;

	for (const auto& entry : files)
	{
		std::string fullPath = std::string(dataDir) + "/" + entry.filename;
		if (!loadFile(fullPath.c_str(), *entry.target, entry.rootKey))
		{
			Logger::Error(std::string("Failed to load: ") + fullPath);
			allOk = false;
		}
		else
		{
			Logger::Info(std::string("Loaded: ") + fullPath);
		}
	}

	// Load optional files (non-fatal if missing)
	for (const auto& entry : optionalFiles)
	{
		std::string fullPath = std::string(dataDir) + "/" + entry.filename;
		if (!loadFile(fullPath.c_str(), *entry.target, entry.rootKey))
		{
			Logger::Warn(std::string("Optional data file not found: ") + fullPath);
		}
		else
		{
			Logger::Info(std::string("Loaded: ") + fullPath);
		}
	}

	// Build lookup indices for O(1) findById
	buildIndex(m_classes,     m_classIndex,     "id");
	buildIndex(m_monsters,    m_monsterIndex,   "id");
	buildIndex(m_itemsBase,   m_itemBaseIndex,  "id");
	buildIndex(m_weaponTypes, m_weaponTypeIndex,"id");
	buildIndex(m_armorTypes,  m_armorTypeIndex, "id");
	buildIndex(m_materials,   m_materialIndex,  "id");
	buildIndex(m_prefixes,    m_prefixIndex,    "id");
	buildIndex(m_postfixes,   m_postfixIndex,   "id");
	buildIndex(m_spells,      m_spellIndex,     "id");
	buildIndex(m_abilities,   m_abilityIndex,   "id");

	return allOk;
}

bool JsonDataManager::loadFile(const char* path, json& out, const char* rootKey)
{
	std::string text = FileReadString(path);
	if (text.empty())
	{
		return false;
	}

	try
	{
		json parsed = json::parse(text);
		if (rootKey != nullptr && parsed.contains(rootKey))
		{
			out = parsed[rootKey];
		}
		else
		{
			out = std::move(parsed);
		}
	}
	catch (const std::exception& e)
	{
		Logger::Error(std::string("JSON parse error in ") + path + ": " + e.what());
		return false;
	}

	return true;
}

void JsonDataManager::buildIndex(const json& array, std::unordered_map<std::string, size_t>& out, const char* key) noexcept
{
	out.clear();
	if (!array.is_array())
	{
		return;
	}
	for (size_t i = 0; i < array.size(); ++i)
	{
		const json& item = array[i];
		if (item.is_object() && item.contains(key))
		{
			out[item[key].get<std::string>()] = i;
		}
	}
}

static const json& getByIndex(const json& array, const std::unordered_map<std::string, size_t>& index, const std::string& id)
{
	auto it = index.find(id);
	if (it != index.end() && it->second < array.size())
	{
		return array[it->second];
	}
	return array;
}

const json& JsonDataManager::GetClassData(const std::string& classId) const
{
	return getByIndex(m_classes, m_classIndex, classId);
}

const json& JsonDataManager::GetMonsterData(const std::string& monsterId) const
{
	return getByIndex(m_monsters, m_monsterIndex, monsterId);
}

const json& JsonDataManager::GetItemBaseData(const std::string& itemId) const
{
	return getByIndex(m_itemsBase, m_itemBaseIndex, itemId);
}

const json& JsonDataManager::GetWeaponType(const std::string& id) const
{
	return getByIndex(m_weaponTypes, m_weaponTypeIndex, id);
}

const json& JsonDataManager::GetArmorType(const std::string& id) const
{
	return getByIndex(m_armorTypes, m_armorTypeIndex, id);
}

const json& JsonDataManager::GetMaterial(const std::string& id) const
{
	return getByIndex(m_materials, m_materialIndex, id);
}

const json& JsonDataManager::GetPrefix(const std::string& id) const
{
	return getByIndex(m_prefixes, m_prefixIndex, id);
}

const json& JsonDataManager::GetPostfix(const std::string& id) const
{
	return getByIndex(m_postfixes, m_postfixIndex, id);
}

const json& JsonDataManager::GetSpellData(const std::string& spellId) const
{
	return getByIndex(m_spells, m_spellIndex, spellId);
}

const json& JsonDataManager::GetAbilityData(const std::string& abilityId) const
{
	return getByIndex(m_abilities, m_abilityIndex, abilityId);
}
