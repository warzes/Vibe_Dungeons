#pragma once

class EngineException final : public std::runtime_error
{
public:
	EngineException(
		std::string_view message,
		std::source_location location = std::source_location::current()
	)
		: std::runtime_error(message.data())
		, m_location(location)
		, m_formatted(std::string(message) + " [" + location.file_name() + ":" + std::to_string(location.line()) + "]")
	{}

	[[nodiscard]] const char* what() const noexcept override
	{
		return m_formatted.c_str();
	}

	[[nodiscard]] const std::source_location& Location() const noexcept
	{
		return m_location;
	}

private:
	std::source_location m_location;
	std::string m_formatted;
};

// helper macro for throwing with source location
#define THROW_ENGINE(msg) throw EngineException((msg), std::source_location::current())
