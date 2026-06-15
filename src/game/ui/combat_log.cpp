#include "stdafx.h"
#include "game/ui/combat_log.h"

void CombatLog::Add(std::string_view msg, glm::vec3 color)
{
	m_entries.push_back({std::string(msg), color});
	if (m_entries.size() > MAX_ENTRIES)
	{
		m_entries.erase(m_entries.begin(),
			m_entries.begin() + static_cast<ptrdiff_t>(m_entries.size() - MAX_ENTRIES));
	}
}

void CombatLog::Clear()
{
	m_entries.clear();
}
