#pragma once

#include "game/overworld/overworld_cell.h"
#include "game/overworld/overworld_location.h"
#include "core/json_alias.h"

#include <vector>
#include <string>
#include <unordered_map>

class Overworld final
{
public:
	void GenerateDefaultMap();
	void LoadFromJSON(const json& mapData);
	void LoadLocations(const json& locData);

	[[nodiscard]] bool IsWalkable(GridPosition pos) const noexcept;
	[[nodiscard]] bool IsWalkable(int32_t row, int32_t col) const noexcept
	{
		return IsWalkable({row, col, 0});
	}

	[[nodiscard]] const OverworldCell& GetCell(GridPosition pos) const noexcept;
	[[nodiscard]] OverworldCell& GetCell(GridPosition pos) noexcept;
	[[nodiscard]] const OverworldCell& GetCell(int32_t row, int32_t col) const noexcept
	{
		return GetCell({row, col, 0});
	}
	[[nodiscard]] OverworldCell& GetCell(int32_t row, int32_t col) noexcept
	{
		return GetCell({row, col, 0});
	}

	[[nodiscard]] bool HasLineOfSight(GridPosition start, GridPosition end) const noexcept;

	[[nodiscard]] static constexpr int32_t Size() noexcept
	{
		return OVERWORLD_SIZE;
	}

	[[nodiscard]] const std::vector<OverworldLocation>& Locations() const noexcept
	{
		return m_locations;
	}

	[[nodiscard]] const OverworldLocation* FindLocationAt(GridPosition pos) const noexcept;

	// Mark a cell as visited for fog-of-war
	void MarkVisited(GridPosition pos) noexcept
	{
		if (InBounds(pos.row, pos.col))
		{
			GetCell(pos).visited = true;
		}
	}

	[[nodiscard]] bool IsVisited(GridPosition pos) const noexcept
	{
		return InBounds(pos.row, pos.col) && GetCell(pos).visited;
	}

	[[nodiscard]] static bool InBounds(int32_t row, int32_t col) noexcept
	{
		return row >= 0 && row < OVERWORLD_SIZE && col >= 0 && col < OVERWORLD_SIZE;
	}

	// Get visible cells within radius (considering LOS)
	[[nodiscard]] std::vector<GridPosition> GetVisibleCells(GridPosition origin, int32_t radius) const noexcept;

private:
	void generateMap();
	void setTerrainRect(int32_t r0, int32_t c0, int32_t r1, int32_t c1, TerrainType t) noexcept;
	void setTerrainLine(int32_t r0, int32_t c0, int32_t r1, int32_t c1, TerrainType t) noexcept;

	std::array<OverworldCell, static_cast<size_t>(OVERWORLD_SIZE) * static_cast<size_t>(OVERWORLD_SIZE)> m_cells{};
	std::vector<OverworldLocation> m_locations;
};
