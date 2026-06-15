#pragma once

#include <cstdint>

namespace Dice
{
	[[nodiscard]] int32_t Roll(int32_t sides);

	[[nodiscard]] int32_t Roll(int32_t count, int32_t sides);
}
