#pragma once

#include "game/dungeon/chunk.h"
#include <random>

enum class DungeonTheme : uint8_t
{
	Catacombs,
	Mine,
	Temple,
	Fortress,
	Cave,
	Lava
};

struct Room final
{
	int32_t row = 0;
	int32_t col = 0;
	int32_t height = 0;
	int32_t width = 0;
};

class DungeonGenerator final
{
public:
	DungeonGenerator(int32_t seed) noexcept;

	void GenerateRandom(
		Chunk& chunk,
		DungeonTheme theme,
		GridPosition& outStart,
		GridPosition& outStairsDown,
		GridPosition& outStairsUp
	) noexcept;

private:
	void generateRoomTemplates(
		Chunk& chunk,
		DungeonTheme theme,
		GridPosition& outStart,
		GridPosition& outStairsDown,
		GridPosition& outStairsUp
	) noexcept;

	void generateRandomWalk(
		Chunk& chunk,
		DungeonTheme theme,
		GridPosition& outStart,
		GridPosition& outStairsDown,
		GridPosition& outStairsUp
	) noexcept;

	void generateCellularAutomata(
		Chunk& chunk,
		DungeonTheme theme,
		GridPosition& outStart,
		GridPosition& outStairsDown,
		GridPosition& outStairsUp
	) noexcept;

	void generateSimpleGrid(
		Chunk& chunk,
		DungeonTheme theme,
		GridPosition& outStart,
		GridPosition& outStairsDown,
		GridPosition& outStairsUp
	) noexcept;

	// Helpers
	void carveRoom(Chunk& chunk, const Room& room) noexcept;
	void carveCorridor(Chunk& chunk, int32_t r1, int32_t c1, int32_t r2, int32_t c2) noexcept;
	[[nodiscard]] bool isWalkable(const Chunk& chunk, int32_t r, int32_t c) const noexcept;
	[[nodiscard]] bool verifyReachable(const Chunk& chunk, GridPosition start, GridPosition goal) const noexcept;
	void assignTheme(DungeonTheme theme, Chunk& chunk) noexcept;
	void placeFeatures(Chunk& chunk, int32_t numDoors, int32_t numTraps, int32_t numSecrets, int32_t numResources) noexcept;
	[[nodiscard]] Room pickRandomRoom(int32_t minSize, int32_t maxSize) noexcept;

	std::mt19937 m_rng;
};
