#pragma once

#include "game/dungeon/chunk.h"

class Dungeon final
{
public:
	void GenerateTestRoom();

	[[nodiscard]] bool IsWalkable(GridPosition pos) const noexcept;
	[[nodiscard]] bool IsWalkable(int32_t row, int32_t col, int32_t floor) const noexcept
	{
		return IsWalkable({row, col, floor});
	}

	[[nodiscard]] const Cell& GetCell(GridPosition pos) const noexcept;
	[[nodiscard]] Cell& GetCell(GridPosition pos) noexcept;

	// Line-of-sight check: bresenham ray from start to end
	// Returns false if a wall/locked door blocks the line
	[[nodiscard]] bool HasLineOfSight(GridPosition start, GridPosition end) const noexcept;

	// Get all cells along a line (for beam/line AoE)
	[[nodiscard]] std::vector<GridPosition> GetLineCells(GridPosition start, GridPosition end) const noexcept;

	[[nodiscard]] const Chunk& GetChunk() const noexcept
	{
		return m_chunk;
	}

	Chunk& GetChunk() noexcept
	{
		return m_chunk;
	}

	void SetChunk(const Chunk& chunk) noexcept
	{
		m_chunk = chunk;
	}

	[[nodiscard]] int32_t ChunkSize() const noexcept
	{
		return Chunk::SIZE;
	}

private:
	Chunk m_chunk;
};
