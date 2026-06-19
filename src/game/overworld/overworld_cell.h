#pragma once

#include <cstdint>
#include <array>
#include "game/grid_position.h"

enum class TerrainType : uint8_t
{
	Grassland,
	Forest,
	Mountain,
	Water,
	Road,
	Desert,
	COUNT
};

constexpr int32_t OVERWORLD_SIZE = 64;

struct OverworldCell final
{
	TerrainType terrain = TerrainType::Grassland;

	bool visited = false;
	bool hasLocation = false;

	[[nodiscard]] bool IsWalkable() const noexcept
	{
		return terrain != TerrainType::Mountain && terrain != TerrainType::Water;
	}

	[[nodiscard]] bool BlocksLineOfSight() const noexcept
	{
		return terrain == TerrainType::Mountain || terrain == TerrainType::Forest;
	}

	[[nodiscard]] float MoveCost() const noexcept
	{
		return (terrain == TerrainType::Road) ? 0.5f : 1.0f;
	}

	[[nodiscard]] float EncounterChance() const noexcept
	{
		switch (terrain)
		{
			case TerrainType::Grassland: return 0.10f;
			case TerrainType::Forest:    return 0.20f;
			case TerrainType::Road:      return 0.02f;
			case TerrainType::Desert:    return 0.08f;
			default:                     return 0.00f;
		}
	}
};
