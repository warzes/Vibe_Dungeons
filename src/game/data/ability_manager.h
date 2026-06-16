#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct AbilityManager final
{
	[[nodiscard]] static const json& GetAbility(const std::string& abilityId);
	[[nodiscard]] static std::vector<std::string> GetAbilitiesByClass(const std::string& classId);
};
