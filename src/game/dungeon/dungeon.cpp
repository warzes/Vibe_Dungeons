#include "stdafx.h"
#include "game/dungeon/dungeon.h"

void Dungeon::GenerateTestRoom()
{
	// Reset all cells
	for (int32_t r = 0; r < Chunk::SIZE; ++r)
	{
		for (int32_t c = 0; c < Chunk::SIZE; ++c)
		{
			m_chunk.At(r, c) = Cell{};
		}
	}

	// Helper
	auto setWall = [&](int32_t r, int32_t c)
	{
		Cell& cell = m_chunk.At(r, c);
		cell.isWall = true;
		cell.hasFloor = false;
		cell.hasCeiling = false;
	};

	// Floor everywhere except walls
	for (int32_t r = 0; r < Chunk::SIZE; ++r)
	{
		for (int32_t c = 0; c < Chunk::SIZE; ++c)
		{
			m_chunk.At(r, c).hasFloor = true;
			m_chunk.At(r, c).hasCeiling = true;
		}
	}

	// Outer boundary walls
	for (int32_t c = 0; c < Chunk::SIZE; ++c)
	{
		setWall(0, c);
		setWall(Chunk::SIZE - 1, c);
	}
	for (int32_t r = 1; r < Chunk::SIZE - 1; ++r)
	{
		setWall(r, 0);
		setWall(r, Chunk::SIZE - 1);
	}

	// Horizontal divider at row 10 with a 3-cell doorway at cols 16-18
	for (int32_t c = 2; c < Chunk::SIZE - 2; ++c)
	{
		if (c >= 16 && c <= 18)
		{
			continue;
		}
		setWall(10, c);
	}

	// Vertical walls in top section — creates 3 rooms
	for (int32_t r = 1; r < 10; ++r)
	{
		setWall(r, 10);
		setWall(r, 24);
	}

	// Pillars in the bottom room for visual depth
	setWall(22, 12);
	setWall(22, 22);
	setWall(28, 17);
}

bool Dungeon::IsWalkable(GridPosition pos) const noexcept
{
	if (!Chunk::InBounds(pos.row, pos.col))
	{
		return false;
	}

	return m_chunk.At(pos.row, pos.col).IsWalkable();
}

const Cell& Dungeon::GetCell(GridPosition pos) const noexcept
{
	if (!Chunk::InBounds(pos.row, pos.col))
	{
		static const Cell s_voidCell{};
		return s_voidCell;
	}

	return m_chunk.At(pos.row, pos.col);
}

Cell& Dungeon::GetCell(GridPosition pos) noexcept
{
	assert(Chunk::InBounds(pos.row, pos.col) && "GetCell non-const out of bounds");
	return m_chunk.At(pos.row, pos.col);
}

bool Dungeon::HasLineOfSight(GridPosition start, GridPosition end) const noexcept
{
	if (start.floor != end.floor)
	{
		return false;
	}

	int32_t dr = end.row - start.row;
	int32_t dc = end.col - start.col;
	int32_t steps = std::max(std::abs(dr), std::abs(dc));
	if (steps == 0)
	{
		return true;
	}

	float rowStep = static_cast<float>(dr) / static_cast<float>(steps);
	float colStep = static_cast<float>(dc) / static_cast<float>(steps);

	for (int32_t i = 1; i < steps; ++i)
	{
		int32_t r = static_cast<int32_t>(std::round(start.row + rowStep * static_cast<float>(i)));
		int32_t c = static_cast<int32_t>(std::round(start.col + colStep * static_cast<float>(i)));

		if (!Chunk::InBounds(r, c))
		{
			return false;
		}

		const Cell& cell = m_chunk.At(r, c);
		if (cell.BlocksLineOfSight())
		{
			return false;
		}
	}

	return true;
}

std::vector<GridPosition> Dungeon::GetLineCells(GridPosition start, GridPosition end) const noexcept
{
	std::vector<GridPosition> cells;

	if (start.floor != end.floor)
	{
		return cells;
	}

	int32_t dr = end.row - start.row;
	int32_t dc = end.col - start.col;
	int32_t steps = std::max(std::abs(dr), std::abs(dc));
	if (steps == 0)
	{
		cells.push_back(start);
		return cells;
	}

	float rowStep = static_cast<float>(dr) / static_cast<float>(steps);
	float colStep = static_cast<float>(dc) / static_cast<float>(steps);

	for (int32_t i = 0; i <= steps; ++i)
	{
		int32_t r = static_cast<int32_t>(std::round(start.row + rowStep * static_cast<float>(i)));
		int32_t c = static_cast<int32_t>(std::round(start.col + colStep * static_cast<float>(i)));

		if (!Chunk::InBounds(r, c))
		{
			break;
		}

		cells.emplace_back(r, c, start.floor);

		if (m_chunk.At(r, c).BlocksLineOfSight())
		{
			break;
		}
	}

	return cells;
}
