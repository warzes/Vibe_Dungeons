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
