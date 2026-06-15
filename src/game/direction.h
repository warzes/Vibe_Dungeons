#pragma once

#include "grid_position.h"

enum class Direction : uint8_t
{
	North,
	East,
	South,
	West
};

[[nodiscard]] inline Direction DirectionFromIndex(int32_t i)
{
	static constexpr Direction DIRECTIONS[4] = {
		Direction::North,
		Direction::East,
		Direction::South,
		Direction::West
	};
	return DIRECTIONS[((i % 4) + 4) % 4];
}

[[nodiscard]] inline Direction NextDirection(Direction d, bool clockwise)
{
	return DirectionFromIndex(static_cast<int32_t>(d) + (clockwise ? 1 : -1));
}

[[nodiscard]] inline glm::ivec2 DirectionToVec(Direction d)
{
	switch (d)
	{
		case Direction::North: return glm::ivec2(-1,  0);
		case Direction::East:  return glm::ivec2( 0,  1);
		case Direction::South: return glm::ivec2( 1,  0);
		case Direction::West:  return glm::ivec2( 0, -1);
	}
	return glm::ivec2(0);
}

[[nodiscard]] inline float DirectionToYaw(Direction d)
{
	switch (d)
	{
		case Direction::North: return -90.0f;
		case Direction::East:  return   0.0f;
		case Direction::South: return  90.0f;
		case Direction::West:  return 180.0f;
	}
	return 0.0f;
}
