#pragma once

struct ProfileSample
{
	std::string name;
	double totalTime = 0.0;
	double minTime = 1e9;
	double maxTime = 0.0;
	int32_t callCount = 0;

	double Average() const noexcept
	{
		return callCount > 0 ? totalTime / callCount : 0.0;
	}
};

class Profiler final
{
public:
	static Profiler& Get() noexcept
	{
		static Profiler instance;
		return instance;
	}

	void BeginFrame() noexcept;
	void EndFrame() noexcept;

	void BeginSample(std::string_view name) noexcept;
	void EndSample() noexcept;

	[[nodiscard]] double FrameTime() const noexcept
	{
		return m_frameTime;
	}

	[[nodiscard]] const std::vector<ProfileSample>& Samples() const noexcept
	{
		return m_samples;
	}

	[[nodiscard]] int32_t FrameCount() const noexcept
	{
		return m_frameCount;
	}

	void Reset() noexcept
	{
		m_samples.clear();
		m_frameCount = 0;
	}

private:
	Profiler() noexcept = default;

	using Clock = std::chrono::steady_clock;

	static constexpr int32_t MAX_DEPTH = 64;

	Clock::time_point m_frameStart;
	double m_frameTime = 0.0;
	int32_t m_frameCount = 0;

	int32_t m_sampleDepth = 0;
	Clock::time_point m_sampleStart[MAX_DEPTH]{};
	size_t m_sampleIndex[MAX_DEPTH]{};

	std::vector<ProfileSample> m_samples;
	std::unordered_map<std::string, size_t> m_sampleMap;
};

class ScopedProfile final
{
public:
	explicit ScopedProfile(std::string_view name) noexcept
	{
		Profiler::Get().BeginSample(name);
	}

	~ScopedProfile() noexcept
	{
		Profiler::Get().EndSample();
	}

	ScopedProfile(const ScopedProfile&) = delete;
	ScopedProfile& operator=(const ScopedProfile&) = delete;
};

#define PROFILE_SCOPE(name) ScopedProfile detail_CONCAT(profScope_, __LINE__)(name)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__func__)

#define detail_CONCAT2(a, b) a##b
#define detail_CONCAT(a, b) detail_CONCAT2(a, b)
