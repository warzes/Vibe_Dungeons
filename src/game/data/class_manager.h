#pragma once

#include <string>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct ClassManager final
{
	[[nodiscard]] static const json& GetClass(const std::string& classId);
	static void ApplyStartingStats(const std::string& classId, int32_t& hp, int32_t& mp,
		int32_t& maxHp, int32_t& maxMp, int32_t& ac, int32_t& atkBonus,
		int32_t& str, int32_t& dex, int32_t& con, int32_t& intel);
};
