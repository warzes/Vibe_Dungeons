#include "stdafx.h"
#include "game/dungeon/dungeon_renderer.h"
#include "game/dungeon/dungeon.h"
#include "engine/renderer/renderer.h"

void DungeonRenderer::Build(const Dungeon& dungeon)
{
	if (!m_dirty)
	{
		return;
	}

	const auto& chunk = dungeon.GetChunk();

	DungeonGeometry geo;
	buildFloorGeometry(chunk, geo);
	buildWallGeometry(chunk, geo);
	buildCeilingGeometry(chunk, geo);

	if (!geo.floorVerts.empty())
	{
		m_floorMesh.Create(geo.floorVerts, geo.floorIndices);
	}
	if (!geo.wallVerts.empty())
	{
		m_wallMesh.Create(geo.wallVerts, geo.wallIndices);
	}
	if (!geo.ceilingVerts.empty())
	{
		m_ceilingMesh.Create(geo.ceilingVerts, geo.ceilingIndices);
	}

	m_dirty = false;
}

void DungeonRenderer::Submit(Renderer& renderer)
{
	const glm::mat4 identity(1.0f);

	if (m_floorMaterial && m_floorMesh)
	{
		renderer.Submit(m_floorMesh, *m_floorMaterial, identity);
	}
	if (m_wallMaterial && m_wallMesh)
	{
		renderer.Submit(m_wallMesh, *m_wallMaterial, identity);
	}
	if (m_ceilingMaterial && m_ceilingMesh)
	{
		renderer.Submit(m_ceilingMesh, *m_ceilingMaterial, identity);
	}
}

void DungeonRenderer::buildFloorGeometry(const Chunk& chunk, DungeonGeometry& out)
{
	for (int32_t r = 0; r < Chunk::SIZE; ++r)
	{
		for (int32_t c = 0; c < Chunk::SIZE; ++c)
		{
			const Cell& cell = chunk.At(r, c);
			if (!cell.hasFloor)
			{
				continue;
			}

			const float x0 = static_cast<float>(c);
			const float x1 = static_cast<float>(c + 1);
			const float z0 = static_cast<float>(r);
			const float z1 = static_cast<float>(r + 1);

			const uint32_t base = static_cast<uint32_t>(out.floorVerts.size());

			out.floorVerts.push_back({ {x0, 0.0f, z0}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f} });
			out.floorVerts.push_back({ {x1, 0.0f, z0}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f} });
			out.floorVerts.push_back({ {x1, 0.0f, z1}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f} });
			out.floorVerts.push_back({ {x0, 0.0f, z1}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f} });

			out.floorIndices.push_back(base + 0);
			out.floorIndices.push_back(base + 2);
			out.floorIndices.push_back(base + 1);
			out.floorIndices.push_back(base + 0);
			out.floorIndices.push_back(base + 3);
			out.floorIndices.push_back(base + 2);
		}
	}
}

void DungeonRenderer::buildCeilingGeometry(const Chunk& chunk, DungeonGeometry& out)
{
	for (int32_t r = 0; r < Chunk::SIZE; ++r)
	{
		for (int32_t c = 0; c < Chunk::SIZE; ++c)
		{
			const Cell& cell = chunk.At(r, c);
			if (!cell.hasCeiling)
			{
				continue;
			}

			const float x0 = static_cast<float>(c);
			const float x1 = static_cast<float>(c + 1);
			const float z0 = static_cast<float>(r);
			const float z1 = static_cast<float>(r + 1);

			const uint32_t base = static_cast<uint32_t>(out.ceilingVerts.size());

			out.ceilingVerts.push_back({ {x0, 1.0f, z0}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f} });
			out.ceilingVerts.push_back({ {x1, 1.0f, z0}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f} });
			out.ceilingVerts.push_back({ {x1, 1.0f, z1}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f} });
			out.ceilingVerts.push_back({ {x0, 1.0f, z1}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f} });

			out.ceilingIndices.push_back(base + 0);
			out.ceilingIndices.push_back(base + 1);
			out.ceilingIndices.push_back(base + 2);
			out.ceilingIndices.push_back(base + 0);
			out.ceilingIndices.push_back(base + 2);
			out.ceilingIndices.push_back(base + 3);
		}
	}
}

void DungeonRenderer::buildWallGeometry(const Chunk& chunk, DungeonGeometry& out)
{
	for (int32_t r = 0; r < Chunk::SIZE; ++r)
	{
		for (int32_t c = 0; c < Chunk::SIZE; ++c)
		{
			const Cell& cell = chunk.At(r, c);
			if (!cell.isWall)
			{
				continue;
			}

			const float x0 = static_cast<float>(c);
			const float x1 = static_cast<float>(c + 1);
			const float z0 = static_cast<float>(r);
			const float z1 = static_cast<float>(r + 1);

			const auto isNotWall = [&](int32_t nr, int32_t nc) -> bool
			{
				if (!Chunk::InBounds(nr, nc))
				{
					return true;
				}
				return !chunk.At(nr, nc).isWall;
			};

			// North face (z = z0, normal = (0,0,-1))
			if (isNotWall(r - 1, c))
			{
				const uint32_t base = static_cast<uint32_t>(out.wallVerts.size());
				out.wallVerts.push_back({ {x0, 0.0f, z0}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f} });
				out.wallVerts.push_back({ {x1, 0.0f, z0}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f} });
				out.wallVerts.push_back({ {x1, 1.0f, z0}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f} });
				out.wallVerts.push_back({ {x0, 1.0f, z0}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f} });
				out.wallIndices.push_back(base + 0);
				out.wallIndices.push_back(base + 2);
				out.wallIndices.push_back(base + 1);
				out.wallIndices.push_back(base + 0);
				out.wallIndices.push_back(base + 3);
				out.wallIndices.push_back(base + 2);
			}

			// South face (z = z1, normal = (0,0,1))
			if (isNotWall(r + 1, c))
			{
				const uint32_t base = static_cast<uint32_t>(out.wallVerts.size());
				out.wallVerts.push_back({ {x1, 0.0f, z1}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f} });
				out.wallVerts.push_back({ {x0, 0.0f, z1}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f} });
				out.wallVerts.push_back({ {x0, 1.0f, z1}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f} });
				out.wallVerts.push_back({ {x1, 1.0f, z1}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f} });
				out.wallIndices.push_back(base + 0);
				out.wallIndices.push_back(base + 2);
				out.wallIndices.push_back(base + 1);
				out.wallIndices.push_back(base + 0);
				out.wallIndices.push_back(base + 3);
				out.wallIndices.push_back(base + 2);
			}

			// West face (x = x0, normal = (-1,0,0))
			if (isNotWall(r, c - 1))
			{
				const uint32_t base = static_cast<uint32_t>(out.wallVerts.size());
				out.wallVerts.push_back({ {x0, 0.0f, z1}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });
				out.wallVerts.push_back({ {x0, 0.0f, z0}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });
				out.wallVerts.push_back({ {x0, 1.0f, z0}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
				out.wallVerts.push_back({ {x0, 1.0f, z1}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
				out.wallIndices.push_back(base + 0);
				out.wallIndices.push_back(base + 2);
				out.wallIndices.push_back(base + 1);
				out.wallIndices.push_back(base + 0);
				out.wallIndices.push_back(base + 3);
				out.wallIndices.push_back(base + 2);
			}

			// East face (x = x1, normal = (1,0,0))
			if (isNotWall(r, c + 1))
			{
				const uint32_t base = static_cast<uint32_t>(out.wallVerts.size());
				out.wallVerts.push_back({ {x1, 0.0f, z0}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f} });
				out.wallVerts.push_back({ {x1, 0.0f, z1}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f} });
				out.wallVerts.push_back({ {x1, 1.0f, z1}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f} });
				out.wallVerts.push_back({ {x1, 1.0f, z0}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f} });
				out.wallIndices.push_back(base + 0);
				out.wallIndices.push_back(base + 2);
				out.wallIndices.push_back(base + 1);
				out.wallIndices.push_back(base + 0);
				out.wallIndices.push_back(base + 3);
				out.wallIndices.push_back(base + 2);
			}
		}
	}
}
