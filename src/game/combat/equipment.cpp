#include "stdafx.h"
#include "game/combat/equipment.h"
#include "game/serialization.h"
#include "core/data_manager.h"

//=============================================================================

int32_t Equipment::slotIndex(EquipmentSlot s) const noexcept
{
	switch (s)
	{
		case EquipmentSlot::Weapon: return 0;
		case EquipmentSlot::Shield: return 1;
		case EquipmentSlot::Head:   return 2;
		case EquipmentSlot::Body:   return 3;
		case EquipmentSlot::Hands:  return 4;
		case EquipmentSlot::Feet:   return 5;
		case EquipmentSlot::Ring:   return 6;
		case EquipmentSlot::Amulet: return 7;
		default:                    return -1;
	}
}

//=============================================================================

const Item* Equipment::Get(EquipmentSlot slot) const noexcept
{
	int32_t idx = slotIndex(slot);
	if (idx < 0 || !m_occupied[idx])
	{
		return nullptr;
	}
	return &m_items[idx];
}

Item* Equipment::Get(EquipmentSlot slot) noexcept
{
	int32_t idx = slotIndex(slot);
	if (idx < 0 || !m_occupied[idx])
	{
		return nullptr;
	}
	return &m_items[idx];
}

//=============================================================================

std::optional<Item> Equipment::Equip(Item item) noexcept
{
	EquipmentSlot targetSlot = item.slot;
	if (targetSlot == EquipmentSlot::None)
	{
		return std::nullopt;
	}

	int32_t idx = slotIndex(targetSlot);
	if (idx < 0)
	{
		return std::nullopt;
	}

	std::optional<Item> previous;
	if (m_occupied[idx])
	{
		previous = m_items[idx];
	}

	m_items[idx] = std::move(item);
	m_occupied[idx] = true;
	return previous;
}

//=============================================================================

std::optional<Item> Equipment::Unequip(EquipmentSlot slot) noexcept
{
	int32_t idx = slotIndex(slot);
	if (idx < 0 || !m_occupied[idx])
	{
		return std::nullopt;
	}

	std::optional<Item> previous = std::move(m_items[idx]);
	m_occupied[idx] = false;
	return previous;
}

//=============================================================================

ItemStats Equipment::GetTotalStats() const noexcept
{
	ItemStats total{};
	for (int32_t i = 0; i < NUM_SLOTS; ++i)
	{
		if (!m_occupied[i])
		{
			continue;
		}
		const Item& it = m_items[i];
		total.damageMin += it.damageMin;
		total.damageMax += it.damageMax;
		total.ac += it.ac;
		total.atkBonus += it.atkBonus;
		total.strBonus += it.strBonus;
		total.dexBonus += it.dexBonus;
		total.conBonus += it.conBonus;
		total.hpBonus += it.hpBonus;
		total.mpBonus += it.mpBonus;
	}
	return total;
}

//=============================================================================

void Equipment::Clear() noexcept
{
	for (int32_t i = 0; i < NUM_SLOTS; ++i)
	{
		m_occupied[i] = false;
	}
}

//=============================================================================

void Equipment::to_json(json& j) const
{
	json arr = json::array();
	for (int32_t i = 0; i < NUM_SLOTS; ++i)
	{
		json entry;
		entry["slot"] = static_cast<uint8_t>(static_cast<int32_t>(i));
		if (m_occupied[i])
		{
			::to_json(entry, m_items[i]);
			entry["occupied"] = true;
		}
		else
		{
			entry["occupied"] = false;
		}
		arr.push_back(std::move(entry));
	}
	j["slots"] = std::move(arr);
}

void Equipment::from_json(const json& j)
{
	Clear();
	const json& arr = j.at("slots");
	for (const auto& entry : arr)
	{
		bool occupied = entry.value("occupied", false);
		if (!occupied)
		{
			continue;
		}
		uint8_t idx = entry.at("slot").get<uint8_t>();
		if (idx >= static_cast<uint8_t>(NUM_SLOTS))
		{
			continue;
		}
		Item item;
		::from_json(entry, item);
		m_items[idx] = std::move(item);
		m_occupied[idx] = true;
	}
}
