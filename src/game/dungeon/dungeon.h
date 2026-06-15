#pragma once

#include "game/dungeon/chunk.h"

class Dungeon final
{
public:
	void GenerateTestRoom();

	[[nodiscard]] bool IsWalkable(GridPosition pos) const noexcept;
	[[nodiscard]] const Cell& GetCell(GridPosition pos) const noexcept;

	[[nodiscard]] const Chunk& GetChunk() const noexcept
	{
		return m_chunk;
	}

	[[nodiscard]] int32_t ChunkSize() const noexcept
	{
		return Chunk::SIZE;
	}

private:
	Chunk m_chunk;
};
