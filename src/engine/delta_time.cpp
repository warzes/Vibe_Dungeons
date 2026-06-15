#include "stdafx.h"
#include "engine/delta_time.h"

DeltaTime::DeltaTime() noexcept
{}

void DeltaTime::Tick() noexcept
{
	const auto now = std::chrono::steady_clock::now();
	if (m_firstTick)
	{
		m_firstTick = false;
		m_lastTick = now;
		m_seconds = 0.0f;
		return;
	}

	const auto duration = now - m_lastTick;
	m_seconds = std::chrono::duration<float>(duration).count();
	m_totalSeconds += m_seconds;
	m_lastTick = now;
}
