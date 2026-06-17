#include "stdafx.h"
#include "game/combat/character.h"

//=============================================================================

void Character::TakeDamage(int32_t amount) noexcept
{
	m_hp -= amount;
	if (m_hp < 0)
	{
		m_hp = 0;
	}
}

//=============================================================================

void Character::Heal(int32_t amount) noexcept
{
	m_hp += amount;
	if (m_hp > m_maxHp)
	{
		m_hp = m_maxHp;
	}
}

//=============================================================================

void Character::SpendMp(int32_t amount) noexcept
{
	m_mp -= amount;
	if (m_mp < 0)
	{
		m_mp = 0;
	}
}

//=============================================================================

void Character::RestoreMp(int32_t amount) noexcept
{
	m_mp += amount;
	if (m_mp > m_maxMp)
	{
		m_mp = m_maxMp;
	}
}
