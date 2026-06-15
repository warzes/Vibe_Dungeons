#include "stdafx.h"
#include "game/combat/inventory.h"

bool Inventory::Add(const Item& item)
{
	if (m_items.size() >= MAX_ITEMS)
	{
		return false;
	}
	m_items.push_back(item);
	return true;
}

bool Inventory::Remove(size_t index)
{
	if (index >= m_items.size())
	{
		return false;
	}
	m_items.erase(m_items.begin() + static_cast<ptrdiff_t>(index));
	return true;
}

void Inventory::Clear()
{
	m_items.clear();
}

const Item* Inventory::Get(size_t index) const noexcept
{
	if (index >= m_items.size())
	{
		return nullptr;
	}
	return &m_items[index];
}

Item* Inventory::Get(size_t index) noexcept
{
	if (index >= m_items.size())
	{
		return nullptr;
	}
	return &m_items[index];
}

size_t Inventory::Size() const noexcept
{
	return m_items.size();
}
