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
	bool isLockedDoor = false;
	bool isOpenDoor = false;
	bool isTrap = false;
	bool isSecretWall = false;
	bool isResourceNode = false;   // for gathering (step 205)
	bool isDisarmed = false;       // trap disarmed (step 291)
	bool isStairDown = false;      // stairs to next floor (step 268)
	bool isStairUp = false;        // stairs to previous floor (step 269)

	[[nodiscard]] bool IsWalkable() const noexcept
	{
		return !isWall && hasFloor && (!isLockedDoor || isOpenDoor);
	}

	[[nodiscard]] bool BlocksLineOfSight() const noexcept
	{
		return isWall || (isLockedDoor && !isOpenDoor);
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
