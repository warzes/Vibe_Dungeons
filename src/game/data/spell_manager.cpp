#include "stdafx.h"
#include "game/data/spell_manager.h"
#include "core/json_data_manager.h"

const json& SpellManager::GetSpell(const std::string& spellId)
{
	return JsonDataManager::Instance().GetSpellData(spellId);
}

std::vector<std::string> SpellManager::GetSpellsByClass(const std::string& classId)
{
	std::vector<std::string> result;
	const json& spells = JsonDataManager::Instance().AllSpells();
	for (const auto& spell : spells)
	{
		if (spell.contains("classReq"))
		{
			for (const auto& req : spell["classReq"])
			{
				if (req.get<std::string>() == classId)
				{
					result.push_back(spell["id"].get<std::string>());
					break;
				}
			}
		}
	}
	return result;
}
