#include "stdafx.h"
#include "core/profiler.h"

void Profiler::BeginFrame() noexcept
{
	m_frameStart = Clock::now();
	m_sampleDepth = 0;
}

void Profiler::EndFrame() noexcept
{
	const auto now = Clock::now();
	m_frameTime = std::chrono::duration<double, std::milli>(now - m_frameStart).count();
	++m_frameCount;
}

void Profiler::BeginSample(std::string_view name) noexcept
{
	if (m_sampleDepth >= MAX_DEPTH)
	{
		return;
	}

	m_sampleStart[m_sampleDepth] = Clock::now();

	std::string key(name);
	auto it = m_sampleMap.find(key);
	if (it != m_sampleMap.end())
	{
		m_sampleIndex[m_sampleDepth] = it->second;
	}
	else
	{
		m_sampleIndex[m_sampleDepth] = m_samples.size();
		m_sampleMap[std::move(key)] = m_samples.size();
		m_samples.push_back(ProfileSample{ .name = std::string(name) });
	}

	++m_sampleDepth;
}

void Profiler::EndSample() noexcept
{
	if (m_sampleDepth <= 0)
	{
		return;
	}

	--m_sampleDepth;

	const auto now = Clock::now();
	const double elapsed = std::chrono::duration<double, std::milli>(
		now - m_sampleStart[m_sampleDepth]).count();

	auto& sample = m_samples[m_sampleIndex[m_sampleDepth]];
	sample.totalTime += elapsed;
	sample.minTime = std::min(sample.minTime, elapsed);
	sample.maxTime = std::max(sample.maxTime, elapsed);
	++sample.callCount;
}
