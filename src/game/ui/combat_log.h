#pragma once

#include <vector>
#include <string>
#include <glm/glm.hpp>

struct LogEntry final
{
	std::string text;
	glm::vec3 color = glm::vec3(1.0f);
};

class CombatLog final
{
public:
	void Add(std::string_view msg, glm::vec3 color = glm::vec3(1.0f));

	void Clear();

	[[nodiscard]] const std::vector<LogEntry>& Entries() const noexcept
	{
		return m_entries;
	}

	static constexpr size_t MAX_ENTRIES = 128;

private:
	std::vector<LogEntry> m_entries;
};
