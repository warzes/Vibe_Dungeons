#pragma once

#include <string>

struct MaterialGenerator final
{
	[[nodiscard]] static std::string AssembleName(
		const std::string& prefixName,
		const std::string& materialName,
		const std::string& baseName,
		const std::string& postfixName
	);
};
