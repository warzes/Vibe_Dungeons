#pragma once

#include <cstdint>
#include <array>
#include "game/grid_position.h"

constexpr int32_t CHUNK_SIZE = 35;

struct Cell final
{
	bool hasFloor = false;
	bool hasCeiling = false;
	bool isWall = false;

	[[nodiscard]] bool IsWalkable() const noexcept
	{
		return !isWall && hasFloor;
	}
};

class Chunk final
{
public:
	static constexpr int32_t SIZE = CHUNK_SIZE;

	Cell& At(int32_t row, int32_t col)
	{
		return m_cells[row * SIZE + col];
	}

	[[nodiscard]] const Cell& At(int32_t row, int32_t col) const
	{
		return m_cells[row * SIZE + col];
	}

	[[nodiscard]] static bool InBounds(int32_t row, int32_t col) noexcept
	{
		return row >= 0 && row < SIZE && col >= 0 && col < SIZE;
	}

private:
	std::array<Cell, static_cast<size_t>(SIZE) * static_cast<size_t>(SIZE)> m_cells{};
};
