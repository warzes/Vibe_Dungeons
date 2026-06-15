#pragma once

#include <atomic>
#ifndef __EMSCRIPTEN__
#	include <mutex>
#endif

enum class LogLevel : uint8_t
{
	Debug = 0,
	Info,
	Warning,
	Error,
	Fatal
};

class Logger final
{
public:
	Logger() = delete;

	[[nodiscard]] static LogLevel GetLevel() noexcept
	{
		return s_level.load(std::memory_order_relaxed);
	}

	static void SetLevel(LogLevel level) noexcept
	{
		s_level.store(level, std::memory_order_relaxed);
	}

	static void Debug(std::string_view msg) noexcept
	{
		write(LogLevel::Debug, msg);
	}

	static void Info(std::string_view msg) noexcept
	{
		write(LogLevel::Info, msg);
	}

	static void Warn(std::string_view msg) noexcept
	{
		write(LogLevel::Warning, msg);
	}

	static void Error(std::string_view msg) noexcept
	{
		write(LogLevel::Error, msg);
	}

	static void Fatal(std::string_view msg) noexcept
	{
		write(LogLevel::Fatal, msg);
	}

private:
	static std::atomic<LogLevel> s_level;
#ifndef __EMSCRIPTEN__
	static std::mutex s_mutex;
#endif

	static void write(LogLevel level, std::string_view msg) noexcept;

	static constexpr const char* levelToString(LogLevel level) noexcept;
};
