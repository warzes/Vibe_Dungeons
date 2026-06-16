#include "stdafx.h"
#include "game/data/ability_manager.h"
#include "core/json_data_manager.h"

const json& AbilityManager::GetAbility(const std::string& abilityId)
{
	return JsonDataManager::Instance().GetAbilityData(abilityId);
}

std::vector<std::string> AbilityManager::GetAbilitiesByClass(const std::string& classId)
{
	std::vector<std::string> result;
	const json& abilities = JsonDataManager::Instance().AllAbilities();
	for (const auto& ab : abilities)
	{
		if (ab.contains("classReq"))
		{
			for (const auto& req : ab["classReq"])
			{
				if (req.get<std::string>() == classId)
				{
					result.push_back(ab["id"].get<std::string>());
					break;
				}
			}
		}
	}
	return result;
}
