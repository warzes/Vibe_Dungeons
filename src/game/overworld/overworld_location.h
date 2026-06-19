#pragma once

#include <string>
#include "game/grid_position.h"

enum class LocationType : uint8_t
{
	Town,
	DungeonEntrance,
	Shrine,
	Camp
};

struct OverworldLocation final
{
	std::string id;
	std::string name;
	LocationType type;
	GridPosition position;
	std::string description;
};
