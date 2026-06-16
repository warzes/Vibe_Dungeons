#include "stdafx.h"
#include "core/json_data_manager.h"
#include "core/logger.h"
#include "core/exception.h"
#include <cstdio>
#include <cstring>

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
		{"abilities.json",    "abilities",   &m_abilities}
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

	return allOk;
}

bool JsonDataManager::loadFile(const char* path, json& out, const char* rootKey)
{
	FILE* fp = fopen(path, "rb");
	if (!fp)
	{
		return false;
	}

	fseek(fp, 0, SEEK_END);
	long size = ftell(fp);
	rewind(fp);

	if (size <= 0)
	{
		fclose(fp);
		return false;
	}

	std::string buffer;
	buffer.resize(static_cast<size_t>(size));
	size_t readBytes = fread(buffer.data(), 1, static_cast<size_t>(size), fp);
	fclose(fp);

	if (static_cast<long>(readBytes) != size)
	{
		return false;
	}

	try
	{
		json parsed = json::parse(buffer);
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

static const json& findById(const json& array, const std::string& id, const char* key)
{
	if (!array.is_array())
	{
		return array;
	}

	for (const auto& item : array)
	{
		if (item.is_object() && item.value(key, std::string()) == id)
		{
			return item;
		}
	}

	return array;
}

const json& JsonDataManager::GetClassData(const std::string& classId) const
{
	return findById(m_classes, classId, "id");
}

const json& JsonDataManager::GetMonsterData(const std::string& monsterId) const
{
	return findById(m_monsters, monsterId, "id");
}

const json& JsonDataManager::GetItemBaseData(const std::string& itemId) const
{
	return findById(m_itemsBase, itemId, "id");
}

const json& JsonDataManager::GetWeaponType(const std::string& id) const
{
	return findById(m_weaponTypes, id, "id");
}

const json& JsonDataManager::GetArmorType(const std::string& id) const
{
	return findById(m_armorTypes, id, "id");
}

const json& JsonDataManager::GetMaterial(const std::string& id) const
{
	return findById(m_materials, id, "id");
}

const json& JsonDataManager::GetPrefix(const std::string& id) const
{
	return findById(m_prefixes, id, "id");
}

const json& JsonDataManager::GetPostfix(const std::string& id) const
{
	return findById(m_postfixes, id, "id");
}

const json& JsonDataManager::GetSpellData(const std::string& spellId) const
{
	return findById(m_spells, spellId, "id");
}

const json& JsonDataManager::GetAbilityData(const std::string& abilityId) const
{
	return findById(m_abilities, abilityId, "id");
}
