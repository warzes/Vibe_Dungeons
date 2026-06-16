#include "stdafx.h"
#include "game/data/material_generator.h"

std::string MaterialGenerator::AssembleName(
	const std::string& prefixName,
	const std::string& materialName,
	const std::string& baseName,
	const std::string& postfixName)
{
	std::string result;

	if (!prefixName.empty())
	{
		result += prefixName + " ";
	}

	if (!materialName.empty())
	{
		result += materialName + " ";
	}

	result += baseName;

	if (!postfixName.empty())
	{
		result += " " + postfixName;
	}

	return result;
}
