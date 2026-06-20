#pragma once

#include "game/dungeon/chunk.h"
#include "game/dungeon/dungeon_generator.h"

class Dungeon final
{
public:
	void GenerateTestRoom();
	void Generate(int32_t seed);
	void NextFloor() noexcept;
	void PrevFloor() noexcept;

	[[nodiscard]] int32_t GetCurrentFloor() const noexcept { return m_currentFloor; }
	[[nodiscard]] DungeonTheme GetTheme() const noexcept { return m_theme; }

	[[nodiscard]] bool IsWalkable(GridPosition pos) const noexcept;
	[[nodiscard]] bool IsWalkable(int32_t row, int32_t col, int32_t floor) const noexcept
	{
		return IsWalkable({row, col, floor});
	}

	[[nodiscard]] const Cell& GetCell(GridPosition pos) const noexcept;
	[[nodiscard]] Cell& GetCell(GridPosition pos) noexcept;

	// Check if a position has stairs
	[[nodiscard]] bool HasStairsDown(GridPosition pos) const noexcept;
	[[nodiscard]] bool HasStairsUp(GridPosition pos) const noexcept;

	// Check if a monster occupies a cell
	void SetMonsterAt(GridPosition pos, bool occupied) noexcept;
	[[nodiscard]] bool IsMonsterAt(GridPosition pos) const noexcept;

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
	void clearMonsterGrid() noexcept;

	Chunk m_chunk;
	int32_t m_currentFloor = 1;
	DungeonTheme m_theme = DungeonTheme::Catacombs;
	GridPosition m_startPos;
	GridPosition m_stairsDown;
	GridPosition m_stairsUp;

	// Monster occupancy grid (separate from Chunk to avoid serialization issues)
	bool m_monsterGrid[Chunk::SIZE][Chunk::SIZE]{};
};
