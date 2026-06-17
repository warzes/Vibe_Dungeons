#pragma once

#include "game/game_mode.h"
#include "game/combat/turn_queue.h"

class Character;

class TurnManager final
{
public:
	[[nodiscard]] GameMode GetGameMode() const noexcept
	{
		return m_gameMode;
	}

	void SetGameMode(GameMode mode) noexcept
	{
		m_gameMode = mode;
	}

	void AdvanceTurn() noexcept
	{
		m_turnQueue.Advance();
	}

	[[nodiscard]] bool IsPlayerTurn() const noexcept
	{
		return m_turnQueue.IsPlayerTurn();
	}

	[[nodiscard]] int32_t CurrentActor() const noexcept
	{
		return m_turnQueue.CurrentActor();
	}

	void EndPlayerTurn(bool isAnimating) noexcept;
	void ApplyRegen(Character& character) const noexcept;

private:
	GameMode m_gameMode = GameMode::Exploring;
	TurnQueue m_turnQueue;
};
