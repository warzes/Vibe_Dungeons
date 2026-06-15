#pragma once

#include "game/grid_position.h"

enum class TileType : uint8_t
{
	Void,
	Floor,
	Wall,
	DoorClosed,
	DoorOpen
};

struct Tile final
{
	TileType type = TileType::Void;
};
