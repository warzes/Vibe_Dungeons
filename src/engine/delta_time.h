#pragma once

class DeltaTime final
{
public:
	DeltaTime() noexcept;

	void Tick() noexcept;

	[[nodiscard]] float Seconds() const noexcept
	{
		return m_seconds;
	}

	[[nodiscard]] float Milliseconds() const noexcept
	{
		return m_seconds * 1000.0f;
	}

	[[nodiscard]] float TotalTime() const noexcept
	{
		return m_totalSeconds;
	}

private:
	std::chrono::steady_clock::time_point m_lastTick;
	float m_seconds = 0.0f;
	float m_totalSeconds = 0.0f;
	bool m_firstTick = true;
};
