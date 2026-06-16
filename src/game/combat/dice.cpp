#include "stdafx.h"
#include "game/combat/dice.h"

static std::mt19937& rng()
{
	static thread_local std::mt19937 s_rng(std::random_device{}());
	return s_rng;
}

int32_t Dice::Roll(int32_t sides)
{
	if (sides < 1)
	{
		return 0;
	}
	std::uniform_int_distribution<int32_t> dist(1, sides);
	return dist(rng());
}

int32_t Dice::Roll(int32_t count, int32_t sides)
{
	int32_t total = 0;
	for (int32_t i = 0; i < count; ++i)
	{
		total += Roll(sides);
	}
	return total;
}
