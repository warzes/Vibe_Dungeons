#include "stdafx.h"
#include "core/logger.h"

std::atomic<LogLevel> Logger::s_level = LogLevel::Debug;
#ifndef __EMSCRIPTEN__
std::mutex Logger::s_mutex;
#endif

void Logger::write(LogLevel level, std::string_view msg) noexcept
{
#ifndef __EMSCRIPTEN__
	std::lock_guard<std::mutex> lock(s_mutex);
#endif
	if (level < s_level.load(std::memory_order_relaxed))
	{
		return;
	}

	auto now = std::chrono::system_clock::now();
	auto tt = std::chrono::system_clock::to_time_t(now);
	std::tm tm {};
#if defined(_MSC_VER) || defined(__MINGW32__)
	localtime_s(&tm, &tt);
#elif defined(__EMSCRIPTEN__)
	// WASM: no localtime_r/s, use non-thread-safe localtime
	std::tm* t = std::localtime(&tt);
	if (t) tm = *t;
#else
	localtime_r(&tt, &tm);
#endif

	std::fprintf(stderr, "[%02d:%02d:%02d] [%s] %.*s\n",
		tm.tm_hour, tm.tm_min, tm.tm_sec,
		levelToString(level),
		static_cast<int>(msg.size()), msg.data());
}

constexpr const char* Logger::levelToString(LogLevel level) noexcept
{
	switch (level)
	{
		case LogLevel::Debug:   return "DEBUG";
		case LogLevel::Info:    return "INFO";
		case LogLevel::Warning: return "WARN";
		case LogLevel::Error:   return "ERROR";
		case LogLevel::Fatal:   return "FATAL";
		default:                return "UNKN";
	}
}
