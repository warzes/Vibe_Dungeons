#pragma once

#include "game/combat/item.h"

#include <array>

class Inventory final
{
public:
	bool Add(const Item& item);
	bool Remove(size_t index);
	void Clear() noexcept;

	[[nodiscard]] const Item* Get(size_t index) const noexcept;
	[[nodiscard]] Item* Get(size_t index) noexcept;
	[[nodiscard]] size_t Size() const noexcept;

	static constexpr size_t MAX_ITEMS = 32;

private:
	std::array<Item, MAX_ITEMS> m_items{};
	size_t m_size = 0;
};
