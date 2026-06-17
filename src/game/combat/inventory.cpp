#include "stdafx.h"
#include "game/combat/inventory.h"

AddResult Inventory::Add(const Item& item)
{
	if (m_size >= MAX_ITEMS)
	{
		return AddResult::Full;
	}
	m_items[m_size] = item;
	++m_size;
	return AddResult::Success;
}

bool Inventory::Remove(size_t index)
{
	if (index >= m_size)
	{
		return false;
	}
	// Shift items left to fill the gap
	for (size_t i = index + 1; i < m_size; ++i)
	{
		m_items[i - 1] = m_items[i];
	}
	--m_size;
	return true;
}

void Inventory::Clear() noexcept
{
	m_size = 0;
}

const Item* Inventory::Get(size_t index) const noexcept
{
	if (index >= m_size)
	{
		return nullptr;
	}
	return &m_items[index];
}

Item* Inventory::Get(size_t index) noexcept
{
	if (index >= m_size)
	{
		return nullptr;
	}
	return &m_items[index];
}

size_t Inventory::Size() const noexcept
{
	return m_size;
}
