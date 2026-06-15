#pragma once

struct GridPosition final
{
	int32_t row = 0;
	int32_t col = 0;
	int32_t floor = 0;

	bool operator==(const GridPosition&) const = default;
};

template<>
struct std::hash<GridPosition>
{
	size_t operator()(const GridPosition& p) const noexcept
	{
		size_t h1 = std::hash<int32_t>{}(p.row);
		size_t h2 = std::hash<int32_t>{}(p.col);
		size_t h3 = std::hash<int32_t>{}(p.floor);
		h1 ^= h2 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
		h1 ^= h3 + 0x9e3779b9 + (h1 << 6) + (h1 >> 2);
		return h1;
	}
};
