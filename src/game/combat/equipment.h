#pragma once

#include <array>
#include <optional>
#include "core/json_alias.h"
#include "game/combat/item.h"

struct ItemStats;

class Equipment final
{
public:
	static constexpr int32_t NUM_SLOTS = 8;

	[[nodiscard]] const Item* Get(EquipmentSlot slot) const noexcept;
	[[nodiscard]] Item* Get(EquipmentSlot slot) noexcept;

	// Returns the previously equipped item (if any)
	std::optional<Item> Equip(Item item) noexcept;
	std::optional<Item> Unequip(EquipmentSlot slot) noexcept;

	[[nodiscard]] ItemStats GetTotalStats() const noexcept;

	void Clear() noexcept;

	void to_json(json& j) const;
	void from_json(const json& j);

private:
	[[nodiscard]] int32_t slotIndex(EquipmentSlot s) const noexcept;

	std::array<Item, NUM_SLOTS> m_items{};
	bool m_occupied[NUM_SLOTS] = {};
};
