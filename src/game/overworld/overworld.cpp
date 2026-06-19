#include "stdafx.h"
#include "game/overworld/overworld.h"
#include "core/logger.h"

void Overworld::GenerateDefaultMap()
{
	generateMap();

	m_cells = {};

	// ---- Water border (2 cells thick) ----
	setTerrainRect(0, 0, OVERWORLD_SIZE - 1, OVERWORLD_SIZE - 1, TerrainType::Water);

	// ---- SW quadrant: Grassland (starting area) ----
	setTerrainRect(30, 0, OVERWORLD_SIZE - 1, 31, TerrainType::Grassland);

	// ---- NE quadrant: Mountains ----
	setTerrainRect(0, 32, 15, OVERWORLD_SIZE - 1, TerrainType::Mountain);

	// ---- E area: Desert strip ----
	setTerrainRect(16, 48, 29, OVERWORLD_SIZE - 1, TerrainType::Desert);

	// ---- Forests scattered ----
	setTerrainRect(30, 36, 45, 50, TerrainType::Forest);
	setTerrainRect(16, 8, 29, 20, TerrainType::Forest);
	setTerrainRect(46, 30, 55, 45, TerrainType::Forest);

	// ---- Road from SW to NE ----
	// Horizontal road along row 56, col 0..32
	setTerrainLine(56, 0, 56, 32, TerrainType::Road);
	// Vertical road along col 32, row 30..56
	setTerrainLine(30, 32, 56, 32, TerrainType::Road);
	// Horizontal road along row 30, col 20..32
	setTerrainLine(30, 20, 30, 32, TerrainType::Road);
	// Vertical road along col 20, row 30..44
	setTerrainLine(30, 20, 44, 20, TerrainType::Road);
	// Horizontal road along row 44, col 10..20
	setTerrainLine(44, 10, 44, 20, TerrainType::Road);

	// ---- Clear terrain at location spots ----
	GetCell(56, 10) = {};  // Town = Grassland
	GetCell(14, 50) = {};  // Dungeon entrance
	GetCell(14, 50).terrain = TerrainType::Mountain;
	GetCell(44, 20) = {};  // Camp = Grassland
	GetCell(38, 14) = {};  // Shrine = Grassland

	// Mark location cells
	GetCell(56, 10).hasLocation = true;
	GetCell(14, 50).hasLocation = true;
	GetCell(44, 20).hasLocation = true;
	GetCell(38, 14).hasLocation = true;
}

void Overworld::LoadFromJSON(const json& /*mapData*/)
{
	// Placeholder: future JSON map loading
	GenerateDefaultMap();
}

void Overworld::LoadLocations(const json& locData)
{
	m_locations.clear();

	if (!locData.contains("locations") || !locData["locations"].is_array())
	{
		Logger::Warn("locations.json: no 'locations' array found");
		return;
	}

	for (const auto& entry : locData["locations"])
	{
		OverworldLocation loc;
		loc.id = entry.value("id", "");
		loc.name = entry.value("name", "");
		std::string typeStr = entry.value("type", "Town");
		loc.position.row = entry.value("row", 0);
		loc.position.col = entry.value("col", 0);
		loc.position.floor = 0;
		loc.description = entry.value("description", "");

		if (typeStr == "Town")             loc.type = LocationType::Town;
		else if (typeStr == "DungeonEntrance") loc.type = LocationType::DungeonEntrance;
		else if (typeStr == "Shrine")      loc.type = LocationType::Shrine;
		else if (typeStr == "Camp")        loc.type = LocationType::Camp;

		m_locations.push_back(std::move(loc));
	}
}

bool Overworld::IsWalkable(GridPosition pos) const noexcept
{
	if (!InBounds(pos.row, pos.col))
	{
		return false;
	}
	return GetCell(pos).IsWalkable();
}

const OverworldCell& Overworld::GetCell(GridPosition pos) const noexcept
{
	if (!InBounds(pos.row, pos.col))
	{
		static constexpr OverworldCell VOID_CELL{};
		return VOID_CELL;
	}
	return m_cells[pos.row * OVERWORLD_SIZE + pos.col];
}

OverworldCell& Overworld::GetCell(GridPosition pos) noexcept
{
	return m_cells[pos.row * OVERWORLD_SIZE + pos.col];
}

bool Overworld::HasLineOfSight(GridPosition start, GridPosition end) const noexcept
{
	int32_t dr = glm::abs(end.row - start.row);
	int32_t dc = glm::abs(end.col - start.col);
	int32_t steps = glm::max(dr, dc);
	if (steps == 0) return true;

	int32_t row = start.row;
	int32_t col = start.col;
	int32_t drow = (end.row > start.row) ? 1 : -1;
	int32_t dcol = (end.col > start.col) ? 1 : -1;

	if (dr >= dc)
	{
		int32_t err = 0;
		for (int32_t i = 0; i < steps; ++i)
		{
			row += drow;
			err += dc;
			if (err >= dr)
			{
				col += dcol;
				err -= dr;
			}
			if (row != end.row || col != end.col)
			{
				if (GetCell(row, col).BlocksLineOfSight())
				{
					return false;
				}
			}
		}
	}
	else
	{
		int32_t err = 0;
		for (int32_t i = 0; i < steps; ++i)
		{
			col += dcol;
			err += dr;
			if (err >= dc)
			{
				row += drow;
				err -= dc;
			}
			if (row != end.row || col != end.col)
			{
				if (GetCell(row, col).BlocksLineOfSight())
				{
					return false;
				}
			}
		}
	}
	return true;
}

const OverworldLocation* Overworld::FindLocationAt(GridPosition pos) const noexcept
{
	for (const auto& loc : m_locations)
	{
		if (loc.position.row == pos.row && loc.position.col == pos.col)
		{
			return &loc;
		}
	}
	return nullptr;
}

std::vector<GridPosition> Overworld::GetVisibleCells(GridPosition origin, int32_t radius) const noexcept
{
	std::vector<GridPosition> visible;

	int32_t r0 = glm::max(0, origin.row - radius);
	int32_t r1 = glm::min(OVERWORLD_SIZE - 1, origin.row + radius);
	int32_t c0 = glm::max(0, origin.col - radius);
	int32_t c1 = glm::min(OVERWORLD_SIZE - 1, origin.col + radius);

	for (int32_t r = r0; r <= r1; ++r)
	{
		for (int32_t c = c0; c <= c1; ++c)
		{
			if (HasLineOfSight(origin, {r, c, 0}))
			{
				visible.push_back({r, c, 0});
			}
		}
	}
	return visible;
}

// ---------------------------------------------------------------------------

void Overworld::generateMap()
{
	// Fill all cells with grassland
	for (auto& cell : m_cells)
	{
		cell = OverworldCell{};
	}
}

void Overworld::setTerrainRect(int32_t r0, int32_t c0, int32_t r1, int32_t c1, TerrainType t) noexcept
{
	for (int32_t r = r0; r <= r1; ++r)
	{
		for (int32_t c = c0; c <= c1; ++c)
		{
			if (InBounds(r, c))
			{
				GetCell(r, c).terrain = t;
			}
		}
	}
}

void Overworld::setTerrainLine(int32_t r0, int32_t c0, int32_t r1, int32_t c1, TerrainType t) noexcept
{
	int32_t dr = (r1 > r0) ? 1 : (r1 < r0) ? -1 : 0;
	int32_t dc = (c1 > c0) ? 1 : (c1 < c0) ? -1 : 0;
	int32_t r = r0;
	int32_t c = c0;
	while (true)
	{
		if (InBounds(r, c))
		{
			GetCell(r, c).terrain = t;
		}
		if (r == r1 && c == c1) break;
		r += dr;
		c += dc;
	}
}
