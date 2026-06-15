#pragma once

#include <cstdint>

enum class GameMode : uint8_t
{
	Exploring,
	TurnWaiting,
	TurnAnimating,
	CombatTurn,
	GameOver
};
