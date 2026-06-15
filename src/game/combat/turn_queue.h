#pragma once

#include <cstdint>

class TurnQueue final
{
public:
	void Advance()
	{
		if (m_actorCount <= 1)
		{
			m_currentActor = 0;
			return;
		}
		m_currentActor = (m_currentActor + 1) % m_actorCount;
	}

	[[nodiscard]] bool IsPlayerTurn() const noexcept
	{
		return m_currentActor == 0;
	}

	[[nodiscard]] int32_t CurrentActor() const noexcept
	{
		return m_currentActor;
	}

	void SetActorCount(int32_t count) noexcept
	{
		m_actorCount = (count > 0) ? count : 1;
	}

private:
	int32_t m_currentActor = 0;
	int32_t m_actorCount = 1;
};
