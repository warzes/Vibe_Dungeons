#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

struct SpellManager final
{
	[[nodiscard]] static const json& GetSpell(const std::string& spellId);
	[[nodiscard]] static std::vector<std::string> GetSpellsByClass(const std::string& classId);
};
