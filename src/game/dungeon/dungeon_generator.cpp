#include "stdafx.h"
#include "game/dungeon/dungeon_generator.h"

#include <vector>
#include <queue>
#include <algorithm>
#include <cassert>

//=============================================================================

static constexpr int32_t SIZE = Chunk::SIZE;

// Pre-defined room templates (5 patterns)
// Each is a 7x7 bool matrix where true = floor
struct RoomTemplate final
{
	int32_t h;
	int32_t w;
	bool data[7][7];
};

static constexpr RoomTemplate TEMPLATE_ROOMS[6] =
{
	// 1: Full rectangle (5x5)
	{
		5, 5,
		{
			{0,0,0,0,0,0,0},
			{0,1,1,1,1,1,0},
			{0,1,1,1,1,1,0},
			{0,1,1,1,1,1,0},
			{0,1,1,1,1,1,0},
			{0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0}
		}
	},
	// 2: L-shaped (5x5)
	{
		5, 5,
		{
			{0,0,0,0,0,0,0},
			{0,1,0,0,0,0,0},
			{0,1,0,0,0,0,0},
			{0,1,1,1,1,0,0},
			{0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0}
		}
	},
	// 3: T-shaped (5x5)
	{
		5, 5,
		{
			{0,0,0,0,0,0,0},
			{0,1,1,1,0,0,0},
			{0,0,1,0,0,0,0},
			{0,0,1,0,0,0,0},
			{0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0}
		}
	},
	// 4: Cross-shaped (5x5)
	{
		5, 5,
		{
			{0,0,0,0,0,0,0},
			{0,0,1,0,0,0,0},
			{0,1,1,1,0,0,0},
			{0,0,1,0,0,0,0},
			{0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0}
		}
	},
	// 5: Circular/octagonal (5x5)
	{
		5, 5,
		{
			{0,0,0,0,0,0,0},
			{0,0,1,1,0,0,0},
			{0,1,1,1,1,0,0},
			{0,0,1,1,0,0,0},
			{0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0}
		}
	},
	// 6: Corridor-style (3x7)
	{
		3, 7,
		{
			{0,0,0,0,0,0,0},
			{0,1,1,1,1,1,1},
			{0,1,0,0,0,0,0},
			{0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0},
			{0,0,0,0,0,0,0}
		}
	}
};

//=============================================================================

DungeonGenerator::DungeonGenerator(int32_t seed) noexcept
	: m_rng(seed)
{}

//=============================================================================

void DungeonGenerator::GenerateRandom(
	Chunk& chunk,
	DungeonTheme theme,
	GridPosition& outStart,
	GridPosition& outStairsDown,
	GridPosition& outStairsUp
) noexcept
{
	// Clear chunk
	for (int32_t r = 0; r < SIZE; ++r)
	{
		for (int32_t c = 0; c < SIZE; ++c)
		{
			chunk.At(r, c) = Cell{};
		}
	}

	std::uniform_int_distribution<int32_t> algoDist(0, 3);
	int32_t algo = algoDist(m_rng);

	switch (algo)
	{
		case 0: generateRoomTemplates(chunk, theme, outStart, outStairsDown, outStairsUp); break;
		case 1: generateRandomWalk(chunk, theme, outStart, outStairsDown, outStairsUp); break;
		case 2: generateCellularAutomata(chunk, theme, outStart, outStairsDown, outStairsUp); break;
		case 3: generateSimpleGrid(chunk, theme, outStart, outStairsDown, outStairsUp); break;
		default: generateRoomTemplates(chunk, theme, outStart, outStairsDown, outStairsUp); break;
	}

	// Verify connectivity — if fails, retry with new seed inside same algo
	int32_t attempts = 0;
	while (!verifyReachable(chunk, outStart, outStairsDown) && attempts < 5)
	{
		// Regenerate same algorithm with new seed
		m_rng = std::mt19937(m_rng());
		switch (algo)
		{
			case 0: generateRoomTemplates(chunk, theme, outStart, outStairsDown, outStairsUp); break;
			case 1: generateRandomWalk(chunk, theme, outStart, outStairsDown, outStairsUp); break;
			case 2: generateCellularAutomata(chunk, theme, outStart, outStairsDown, outStairsUp); break;
			case 3: generateSimpleGrid(chunk, theme, outStart, outStairsDown, outStairsUp); break;
		}
		++attempts;
	}
}

//=============================================================================
// Algorithm 1: Room Templates
//=============================================================================

void DungeonGenerator::generateRoomTemplates(
	Chunk& chunk,
	DungeonTheme theme,
	GridPosition& outStart,
	GridPosition& outStairsDown,
	GridPosition& outStairsUp
) noexcept
{
	// Place 5-8 template rooms
	std::uniform_int_distribution<int32_t> roomCount(5, 8);
	std::uniform_int_distribution<int32_t> templatePick(0, 5);
	std::uniform_int_distribution<int32_t> posDist(1, SIZE - 8);

	int32_t numRooms = roomCount(m_rng);
	std::vector<Room> rooms;

	for (int32_t i = 0; i < numRooms; ++i)
	{
		int32_t ti = templatePick(m_rng);
		const auto& tmpl = TEMPLATE_ROOMS[ti];
		int32_t r = posDist(m_rng);
		int32_t c = posDist(m_rng);

		// Check for overlap with existing rooms
		bool overlaps = false;
		for (int32_t dr = -1; dr <= tmpl.h; ++dr)
		{
			for (int32_t dc = -1; dc <= tmpl.w; ++dc)
			{
				int32_t nr = r + dr;
				int32_t nc = c + dc;
				if (Chunk::InBounds(nr, nc) && chunk.At(nr, nc).hasFloor)
				{
					overlaps = true;
					break;
				}
			}
			if (overlaps) break;
		}
		if (overlaps) { --i; continue; }

		// Place the template
		for (int32_t dr = 0; dr < tmpl.h; ++dr)
		{
			for (int32_t dc = 0; dc < tmpl.w; ++dc)
			{
				if (tmpl.data[dr][dc])
				{
					int32_t nr = r + dr;
					int32_t nc = c + dc;
					if (Chunk::InBounds(nr, nc))
					{
						chunk.At(nr, nc).hasFloor = true;
						chunk.At(nr, nc).hasCeiling = true;
					}
				}
			}
		}
		rooms.emplace_back(r, c, tmpl.h, tmpl.w);

		// Connect to previous room with L-corridor
		if (rooms.size() > 1)
		{
			const Room& prev = rooms[rooms.size() - 2];
			int32_t prevCenterR = prev.row + prev.height / 2;
			int32_t prevCenterC = prev.col + prev.width / 2;
			int32_t curCenterR = r + tmpl.h / 2;
			int32_t curCenterC = c + tmpl.w / 2;
			carveCorridor(chunk, prevCenterR, prevCenterC, curCenterR, curCenterC);
		}
	}

	// Set outer boundary walls
	for (int32_t c = 0; c < SIZE; ++c)
	{
		chunk.At(0, c) = Cell{};
		chunk.At(SIZE - 1, c) = Cell{};
	}
	for (int32_t r = 1; r < SIZE - 1; ++r)
	{
		chunk.At(r, 0) = Cell{};
		chunk.At(r, SIZE - 1) = Cell{};
	}

	// Set start and stairs
	if (!rooms.empty())
	{
		outStart = {rooms[0].row + rooms[0].height / 2, rooms[0].col + rooms[0].width / 2, 0};
		const Room& last = rooms.back();
		outStairsDown = {last.row + last.height / 2, last.col + last.width / 2, 0};
	}
	else
	{
		outStart = {SIZE / 2, SIZE / 2, 0};
		outStairsDown = {SIZE / 2 + 1, SIZE / 2 + 1, 0};
	}
	outStairsUp = outStart;

	chunk.At(outStairsDown.row, outStairsDown.col).isStairDown = true;
	chunk.At(outStairsUp.row, outStairsUp.col).isStairUp = true;

	placeFeatures(chunk, 2, 3, 2, 3);
	assignTheme(theme, chunk);
}

//=============================================================================
// Algorithm 2: Random Walk (Drunken Miner)
//=============================================================================

void DungeonGenerator::generateRandomWalk(
	Chunk& chunk,
	DungeonTheme theme,
	GridPosition& outStart,
	GridPosition& outStairsDown,
	GridPosition& outStairsUp
) noexcept
{
	// Start from center
	int32_t r = SIZE / 2;
	int32_t c = SIZE / 2;

	// Carve starting area
	for (int32_t dr = -2; dr <= 2; ++dr)
	{
		for (int32_t dc = -2; dc <= 2; ++dc)
		{
			int32_t nr = r + dr;
			int32_t nc = c + dc;
			if (Chunk::InBounds(nr, nc))
			{
				chunk.At(nr, nc).hasFloor = true;
				chunk.At(nr, nc).hasCeiling = true;
			}
		}
	}

	// Random walk: carve tunnels, expand rooms at intervals
	std::uniform_int_distribution<int32_t> dirDist(0, 3);
	std::uniform_int_distribution<int32_t> stepDist(3, 8);
	std::uniform_int_distribution<int32_t> roomSize(2, 4);

	int32_t carvedCells = 25;
	int32_t targetCells = SIZE * SIZE / 2; // cover 50%
	int32_t stepsSinceRoom = 0;

	while (carvedCells < targetCells)
	{
		// Take a step
		int32_t dir = dirDist(m_rng);
		int32_t nr = r;
		int32_t nc = c;

		switch (dir)
		{
			case 0: nr = r - 1; break; // up
			case 1: nr = r + 1; break; // down
			case 2: nc = c - 1; break; // left
			case 3: nc = c + 1; break; // right
		}

		if (!Chunk::InBounds(nr, nc))
		{
			// Bounce
			r = SIZE / 2;
			c = SIZE / 2;
			continue;
		}

		r = nr;
		c = nc;

		if (!chunk.At(r, c).hasFloor)
		{
			chunk.At(r, c).hasFloor = true;
			chunk.At(r, c).hasCeiling = true;
			++carvedCells;
		}

		++stepsSinceRoom;

		// Expand into a room at intervals
		if (stepsSinceRoom > stepDist(m_rng) && carvedCells < targetCells)
		{
			int32_t rs = roomSize(m_rng);
			for (int32_t dr = -rs; dr <= rs; ++dr)
			{
				for (int32_t dc = -rs; dc <= rs; ++dc)
				{
					int32_t er = r + dr;
					int32_t ec = c + dc;
					if (Chunk::InBounds(er, ec) && !chunk.At(er, ec).hasFloor)
					{
						chunk.At(er, ec).hasFloor = true;
						chunk.At(er, ec).hasCeiling = true;
						++carvedCells;
					}
				}
			}
			stepsSinceRoom = 0;
		}

		// Occasionally teleport to a random spot to avoid getting stuck
		if (stepsSinceRoom > 50)
		{
			r = SIZE / 3 + (m_rng() % (SIZE / 3));
			c = SIZE / 3 + (m_rng() % (SIZE / 3));
			stepsSinceRoom = 0;
		}
	}

	// Walls: any non-floor cell is a wall
	for (int32_t rr = 0; rr < SIZE; ++rr)
	{
		for (int32_t cc = 0; cc < SIZE; ++cc)
		{
			if (!chunk.At(rr, cc).hasFloor)
			{
				chunk.At(rr, cc).isWall = true;
			}
		}
	}

	// Guarantee boundary walls
	for (int32_t cc = 0; cc < SIZE; ++cc)
	{
		chunk.At(0, cc) = Cell{};
		chunk.At(SIZE - 1, cc) = Cell{};
	}
	for (int32_t rr = 1; rr < SIZE - 1; ++rr)
	{
		chunk.At(rr, 0) = Cell{};
		chunk.At(rr, SIZE - 1) = Cell{};
	}

	outStart = {SIZE / 2, SIZE / 2, 0};

	// Find farthest walkable cell from start for stairs
	int32_t bestDist = -1;
	int32_t stairsR = SIZE / 2 + 1;
	int32_t stairsC = SIZE / 2 + 1;
	for (int32_t rr = 2; rr < SIZE - 2; ++rr)
	{
		for (int32_t cc = 2; cc < SIZE - 2; ++cc)
		{
			if (chunk.At(rr, cc).hasFloor)
			{
				int32_t d = std::abs(rr - SIZE / 2) + std::abs(cc - SIZE / 2);
				if (d > bestDist)
				{
					bestDist = d;
					stairsR = rr;
					stairsC = cc;
				}
			}
		}
	}
	outStairsDown = {stairsR, stairsC, 0};
	outStairsUp = outStart;

	chunk.At(outStairsDown.row, outStairsDown.col).isStairDown = true;
	chunk.At(outStairsUp.row, outStairsUp.col).isStairUp = true;

	placeFeatures(chunk, 3, 5, 2, 4);
	assignTheme(theme, chunk);
}

//=============================================================================
// Algorithm 3: Cellular Automata
//=============================================================================

void DungeonGenerator::generateCellularAutomata(
	Chunk& chunk,
	DungeonTheme theme,
	GridPosition& outStart,
	GridPosition& outStairsDown,
	GridPosition& outStairsUp
) noexcept
{
	// Step 1: Random noise initialization (45% walls)
	std::uniform_int_distribution<int32_t> noiseDist(0, 99);

	for (int32_t r = 0; r < SIZE; ++r)
	{
		for (int32_t c = 0; c < SIZE; ++c)
		{
			chunk.At(r, c).hasFloor = true;
			chunk.At(r, c).hasCeiling = true;

			if (noiseDist(m_rng) < 45 && r > 1 && r < SIZE - 2 && c > 1 && c < SIZE - 2)
			{
				chunk.At(r, c).isWall = true;
				chunk.At(r, c).hasFloor = false;
			}
		}
	}

	// Step 2: Cellular automata iterations (5 iterations, B678/S345678)
	for (int32_t iter = 0; iter < 5; ++iter)
	{
		std::vector<std::vector<bool>> nextWalls(
			SIZE, std::vector<bool>(SIZE, false)
		);

		for (int32_t r = 1; r < SIZE - 1; ++r)
		{
			for (int32_t c = 1; c < SIZE - 1; ++c)
			{
				int32_t wallCount = 0;
				for (int32_t dr = -1; dr <= 1; ++dr)
				{
					for (int32_t dc = -1; dc <= 1; ++dc)
					{
						if (dr == 0 && dc == 0) continue;
						int32_t nr = r + dr;
						int32_t nc = c + dc;
						if (nr < 0 || nr >= SIZE || nc < 0 || nc >= SIZE)
						{
							++wallCount; // treat borders as walls
						}
						else if (chunk.At(nr, nc).isWall)
						{
							++wallCount;
						}
					}
				}

				bool isWall = chunk.At(r, c).isWall;
				// Rule: wall stays if 5+ neighbors are walls, floor stays if 4+ neighbors are floor
				if (isWall)
				{
					nextWalls[r][c] = (wallCount >= 4);
				}
				else
				{
					nextWalls[r][c] = (wallCount >= 5);
				}
			}
		}

		// Apply
		for (int32_t r = 1; r < SIZE - 1; ++r)
		{
			for (int32_t c = 1; c < SIZE - 1; ++c)
			{
				chunk.At(r, c).isWall = nextWalls[r][c];
				chunk.At(r, c).hasFloor = !nextWalls[r][c];
				chunk.At(r, c).hasCeiling = !nextWalls[r][c];
			}
		}
	}

	// Step 3: Flood-fill to find largest connected region
	std::vector<std::vector<bool>> visited(
		SIZE, std::vector<bool>(SIZE, false)
	);
	std::vector<std::vector<GridPosition>> regions;

	for (int32_t r = 1; r < SIZE - 1; ++r)
	{
		for (int32_t c = 1; c < SIZE - 1; ++c)
		{
			if (chunk.At(r, c).hasFloor && !visited[r][c])
			{
				std::vector<GridPosition> region;
				std::queue<GridPosition> q;
				q.emplace(r, c, 0);
				visited[r][c] = true;

				while (!q.empty())
				{
					GridPosition pos = q.front();
					q.pop();
					region.push_back(pos);

					static const int32_t DIRS[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
					for (const auto& d : DIRS)
					{
						int32_t nr = pos.row + d[0];
						int32_t nc = pos.col + d[1];
						if (Chunk::InBounds(nr, nc) &&
							chunk.At(nr, nc).hasFloor &&
							!visited[nr][nc])
						{
							visited[nr][nc] = true;
							q.emplace(nr, nc, 0);
						}
					}
				}

				regions.push_back(region);
			}
		}
	}

	// Keep only the largest region, clear the rest
	if (!regions.empty())
	{
		auto largestIt = std::max_element(
			regions.begin(), regions.end(),
			[](const auto& a, const auto& b) { return a.size() < b.size(); }
		);

		// Clear all cells not in the largest region
		for (int32_t r = 0; r < SIZE; ++r)
		{
			for (int32_t c = 0; c < SIZE; ++c)
			{
				if (chunk.At(r, c).hasFloor)
				{
					bool inLargest = false;
					for (const auto& pos : *largestIt)
					{
						if (pos.row == r && pos.col == c)
						{
							inLargest = true;
							break;
						}
					}
					if (!inLargest)
					{
						chunk.At(r, c).isWall = true;
						chunk.At(r, c).hasFloor = false;
						chunk.At(r, c).hasCeiling = false;
					}
				}
			}
		}

		// Start at first cell of largest region
		outStart = {largestIt->front().row, largestIt->front().col, 0};
		outStairsDown = {largestIt->back().row, largestIt->back().col, 0};
	}
	else
	{
		outStart = {SIZE / 2, SIZE / 2, 0};
		outStairsDown = {SIZE / 2 + 1, SIZE / 2 + 1, 0};
	}

	outStairsUp = outStart;

	// Guarantee boundary walls
	for (int32_t cc = 0; cc < SIZE; ++cc)
	{
		chunk.At(0, cc) = Cell{};
		chunk.At(SIZE - 1, cc) = Cell{};
	}
	for (int32_t rr = 1; rr < SIZE - 1; ++rr)
	{
		chunk.At(rr, 0) = Cell{};
		chunk.At(rr, SIZE - 1) = Cell{};
	}

	chunk.At(outStairsDown.row, outStairsDown.col).isStairDown = true;
	chunk.At(outStairsUp.row, outStairsUp.col).isStairUp = true;

	placeFeatures(chunk, 2, 6, 3, 3);
	assignTheme(theme, chunk);
}

//=============================================================================
// Algorithm 4: Simple Grid Placement
//=============================================================================

void DungeonGenerator::generateSimpleGrid(
	Chunk& chunk,
	DungeonTheme theme,
	GridPosition& outStart,
	GridPosition& outStairsDown,
	GridPosition& outStairsUp
) noexcept
{
	// Divide 35x35 into a 5x5 grid of 7x7 cells
	constexpr int32_t GRID_DIVS = 5;
	constexpr int32_t CELL_SIZE = SIZE / GRID_DIVS; // 7

	std::uniform_int_distribution<int32_t> roomH(3, 5);
	std::uniform_int_distribution<int32_t> roomW(3, 5);

	std::vector<std::vector<Room>> gridRooms(GRID_DIVS, std::vector<Room>(GRID_DIVS));

	// Place a room in each grid cell
	for (int32_t gr = 0; gr < GRID_DIVS; ++gr)
	{
		for (int32_t gc = 0; gc < GRID_DIVS; ++gc)
		{
			int32_t baseR = gr * CELL_SIZE + 1;
			int32_t baseC = gc * CELL_SIZE + 1;
			int32_t maxH = std::min(roomH(m_rng), CELL_SIZE - 2);
			int32_t maxW = std::min(roomW(m_rng), CELL_SIZE - 2);

			std::uniform_int_distribution<int32_t> offsetR(0, CELL_SIZE - maxH - 1);
			std::uniform_int_distribution<int32_t> offsetC(0, CELL_SIZE - maxW - 1);
			int32_t orr = offsetR(m_rng);
			int32_t occ = offsetC(m_rng);

			Room room;
			room.row = baseR + orr;
			room.col = baseC + occ;
			room.height = maxH;
			room.width = maxW;

			carveRoom(chunk, room);
			gridRooms[gr][gc] = room;
		}
	}

	// Connect adjacent grid cells with L-corridors
	for (int32_t gr = 0; gr < GRID_DIVS; ++gr)
	{
		for (int32_t gc = 0; gc < GRID_DIVS; ++gc)
		{
			const Room& current = gridRooms[gr][gc];

			// Connect right neighbor
			if (gc + 1 < GRID_DIVS)
			{
				const Room& right = gridRooms[gr][gc + 1];
				int32_t r1 = current.row + current.height / 2;
				int32_t c1 = current.col + current.width - 1;
				int32_t r2 = right.row + right.height / 2;
				int32_t c2 = right.col;
				carveCorridor(chunk, r1, c1, r2, c2);
			}

			// Connect bottom neighbor
			if (gr + 1 < GRID_DIVS)
			{
				const Room& below = gridRooms[gr + 1][gc];
				int32_t r1 = current.row + current.height - 1;
				int32_t c1 = current.col + current.width / 2;
				int32_t r2 = below.row;
				int32_t c2 = below.col + below.width / 2;
				carveCorridor(chunk, r1, c1, r2, c2);
			}
		}
	}

	// Boundary walls
	for (int32_t cc = 0; cc < SIZE; ++cc)
	{
		chunk.At(0, cc) = Cell{};
		chunk.At(SIZE - 1, cc) = Cell{};
	}
	for (int32_t rr = 1; rr < SIZE - 1; ++rr)
	{
		chunk.At(rr, 0) = Cell{};
		chunk.At(rr, SIZE - 1) = Cell{};
	}

	// Set boundary cells as walls where no floor
	for (int32_t rr = 0; rr < SIZE; ++rr)
	{
		for (int32_t cc = 0; cc < SIZE; ++cc)
		{
			if (!chunk.At(rr, cc).hasFloor)
			{
				chunk.At(rr, cc).isWall = true;
			}
		}
	}

	outStart = {gridRooms[0][0].row + gridRooms[0][0].height / 2,
				gridRooms[0][0].col + gridRooms[0][0].width / 2, 0};
	outStairsDown = {gridRooms[GRID_DIVS - 1][GRID_DIVS - 1].row +
						gridRooms[GRID_DIVS - 1][GRID_DIVS - 1].height / 2,
					 gridRooms[GRID_DIVS - 1][GRID_DIVS - 1].col +
					 	gridRooms[GRID_DIVS - 1][GRID_DIVS - 1].width / 2, 0};
	outStairsUp = outStart;

	chunk.At(outStairsDown.row, outStairsDown.col).isStairDown = true;
	chunk.At(outStairsUp.row, outStairsUp.col).isStairUp = true;

	placeFeatures(chunk, 4, 4, 3, 5);
	assignTheme(theme, chunk);
}

//=============================================================================
// Helper: carve a rectangular room
//=============================================================================

void DungeonGenerator::carveRoom(Chunk& chunk, const Room& room) noexcept
{
	for (int32_t r = room.row; r < room.row + room.height; ++r)
	{
		for (int32_t c = room.col; c < room.col + room.width; ++c)
		{
			if (Chunk::InBounds(r, c))
			{
				chunk.At(r, c).hasFloor = true;
				chunk.At(r, c).hasCeiling = true;
				chunk.At(r, c).isWall = false;
			}
		}
	}
}

//=============================================================================
// Helper: carve an L-shaped corridor between two points
//=============================================================================

void DungeonGenerator::carveCorridor(
	Chunk& chunk,
	int32_t r1, int32_t c1,
	int32_t r2, int32_t c2
) noexcept
{
	// Horizontal then vertical
	int32_t curR = r1;
	int32_t curC = c1;

	int32_t stepR = (r2 > r1) ? 1 : (r2 < r1 ? -1 : 0);
	int32_t stepC = (c2 > c1) ? 1 : (c2 < c1 ? -1 : 0);

	// First horizontal
	while (curC != c2)
	{
		if (Chunk::InBounds(curR, curC))
		{
			chunk.At(curR, curC).hasFloor = true;
			chunk.At(curR, curC).hasCeiling = true;
			chunk.At(curR, curC).isWall = false;
		}
		// Carve 2-wide corridor
		if (Chunk::InBounds(curR + 1, curC))
		{
			chunk.At(curR + 1, curC).hasFloor = true;
			chunk.At(curR + 1, curC).hasCeiling = true;
			chunk.At(curR + 1, curC).isWall = false;
		}
		curC += stepC;
	}

	// Then vertical
	while (curR != r2)
	{
		if (Chunk::InBounds(curR, curC))
		{
			chunk.At(curR, curC).hasFloor = true;
			chunk.At(curR, curC).hasCeiling = true;
			chunk.At(curR, curC).isWall = false;
		}
		if (Chunk::InBounds(curR, curC + 1))
		{
			chunk.At(curR, curC + 1).hasFloor = true;
			chunk.At(curR, curC + 1).hasCeiling = true;
			chunk.At(curR, curC + 1).isWall = false;
		}
		curR += stepR;
	}
}

//=============================================================================
// Helper: check if a cell is walkable for BFS
//=============================================================================

bool DungeonGenerator::isWalkable(const Chunk& chunk, int32_t r, int32_t c) const noexcept
{
	if (!Chunk::InBounds(r, c)) return false;
	return chunk.At(r, c).IsWalkable();
}

//=============================================================================
// Helper: BFS connectivity check
//=============================================================================

bool DungeonGenerator::verifyReachable(
	const Chunk& chunk,
	GridPosition start,
	GridPosition goal
) const noexcept
{
	if (!isWalkable(chunk, start.row, start.col)) return false;
	if (!isWalkable(chunk, goal.row, goal.col)) return false;

	std::vector<std::vector<bool>> visited(
		SIZE, std::vector<bool>(SIZE, false)
	);
	std::queue<GridPosition> q;
	q.push(start);
	visited[start.row][start.col] = true;

	static const int32_t DIRS[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};

	while (!q.empty())
	{
		GridPosition cur = q.front();
		q.pop();

		if (cur.row == goal.row && cur.col == goal.col)
		{
			return true;
		}

		for (const auto& d : DIRS)
		{
			int32_t nr = cur.row + d[0];
			int32_t nc = cur.col + d[1];
			if (Chunk::InBounds(nr, nc) && !visited[nr][nc] &&
				isWalkable(chunk, nr, nc))
			{
				visited[nr][nc] = true;
				q.emplace(nr, nc, 0);
			}
		}
	}

	return false;
}

//=============================================================================
// Helper: assign theme to chunk (visual — wall/floor colors via Cell flags)
//=============================================================================

void DungeonGenerator::assignTheme(DungeonTheme theme, Chunk& chunk) noexcept
{
	// Currently themes are purely visual hints stored in a future extension.
	// Theme affects resource distribution but not cell state at this level.
	(void)chunk;
	(void)theme;
}

//=============================================================================
// Helper: place locked doors, traps, secret walls, resource nodes
//=============================================================================

void DungeonGenerator::placeFeatures(
	Chunk& chunk,
	int32_t numDoors,
	int32_t numTraps,
	int32_t numSecrets,
	int32_t numResources
) noexcept
{
	std::uniform_int_distribution<int32_t> posDist(2, SIZE - 3);

	// Collect walkable positions
	std::vector<GridPosition> walkable;
	for (int32_t r = 2; r < SIZE - 2; ++r)
	{
		for (int32_t c = 2; c < SIZE - 2; ++c)
		{
			if (chunk.At(r, c).IsWalkable() &&
				!chunk.At(r, c).isStairDown &&
				!chunk.At(r, c).isStairUp)
			{
				walkable.emplace_back(r, c, 0);
			}
		}
	}

	if (walkable.empty()) return;
	std::shuffle(walkable.begin(), walkable.end(), m_rng);

	size_t idx = 0;

	// Place locked doors on walkable cells
	for (int32_t i = 0; i < numDoors && idx < walkable.size(); ++i, ++idx)
	{
		chunk.At(walkable[idx].row, walkable[idx].col).isLockedDoor = true;
	}

	// Place traps
	for (int32_t i = 0; i < numTraps && idx < walkable.size(); ++i, ++idx)
	{
		chunk.At(walkable[idx].row, walkable[idx].col).isTrap = true;
	}

	// Place secret walls (replace a wall cell adjacent to walkable area)
	for (int32_t i = 0; i < numSecrets; ++i)
	{
		// Find a wall cell with at least one floor neighbor
		for (int32_t attempt = 0; attempt < 50; ++attempt)
		{
			int32_t rr = posDist(m_rng);
			int32_t cc = posDist(m_rng);
			if (!chunk.At(rr, cc).hasFloor || chunk.At(rr, cc).isWall)
			{
				// Check if adjacent to walkable
				bool adjWalkable = false;
				static const int32_t DIRS[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
				for (const auto& d : DIRS)
				{
					int32_t nr = rr + d[0];
					int32_t nc = cc + d[1];
					if (Chunk::InBounds(nr, nc) && chunk.At(nr, nc).IsWalkable())
					{
						adjWalkable = true;
						break;
					}
				}
				if (adjWalkable)
				{
					chunk.At(rr, cc).isSecretWall = true;
					chunk.At(rr, cc).isWall = true;
					chunk.At(rr, cc).hasFloor = false;
					break;
				}
			}
		}
	}

	// Place resource nodes
	for (int32_t i = 0; i < numResources && idx < walkable.size(); ++i, ++idx)
	{
		chunk.At(walkable[idx].row, walkable[idx].col).isResourceNode = true;
	}
}

//=============================================================================
// Helper: pick a random room with given size constraints
//=============================================================================

Room DungeonGenerator::pickRandomRoom(int32_t minSize, int32_t maxSize) noexcept
{
	std::uniform_int_distribution<int32_t> sizeDist(minSize, maxSize);
	Room room;
	room.height = sizeDist(m_rng);
	room.width = sizeDist(m_rng);
	return room;
}
