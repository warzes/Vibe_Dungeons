#include "stdafx.h"
#include "game/overworld/overworld_renderer.h"
#include "game/overworld/overworld.h"
#include "engine/renderer/renderer.h"

// Palette has 8 pixels; we use index 0..5 for terrain types
static constexpr float PALETTE_W = 8.0f;

static float paletteU(int32_t terrainIdx) noexcept
{
	return (static_cast<float>(terrainIdx) + 0.5f) / PALETTE_W;
}

void OverworldRenderer::Build(const Overworld& overworld)
{
	if (!m_dirty)
	{
		return;
	}

	OverworldGeometry geo;
	buildFloorGeometry(overworld, geo);
	buildWallGeometry(overworld, geo);
	buildLocationGeometry(overworld, geo);

	if (!geo.floorVerts.empty())
	{
		m_floorMesh.Create(geo.floorVerts, geo.floorIndices);
	}
	if (!geo.wallVerts.empty())
	{
		m_wallMesh.Create(geo.wallVerts, geo.wallIndices);
	}
	if (!geo.locationVerts.empty())
	{
		m_locationMesh.Create(geo.locationVerts, geo.locationIndices);
	}

	m_dirty = false;
}

void OverworldRenderer::Submit(Renderer& renderer)
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
	if (m_locationMaterial && m_locationMesh)
	{
		renderer.Submit(m_locationMesh, *m_locationMaterial, identity);
	}
}

//=============================================================================

void OverworldRenderer::buildFloorGeometry(const Overworld& overworld, OverworldGeometry& out)
{
	for (int32_t r = 0; r < OVERWORLD_SIZE; ++r)
	{
		for (int32_t c = 0; c < OVERWORLD_SIZE; ++c)
		{
			const OverworldCell& cell = overworld.GetCell(r, c);
			if (!cell.IsWalkable() && cell.terrain != TerrainType::Water)
			{
				continue;
			}

			const float x0 = static_cast<float>(c);
			const float x1 = static_cast<float>(c + 1);
			const float z0 = static_cast<float>(r);
			const float z1 = static_cast<float>(r + 1);

			float vShift = 0.0f;
			int32_t palIdx = 0;
			switch (cell.terrain)
			{
				case TerrainType::Grassland: palIdx = 0; vShift = 0.0f; break;
				case TerrainType::Forest:    palIdx = 1; vShift = 0.0f; break;
				case TerrainType::Water:     palIdx = 3; vShift = -0.05f; break;
				case TerrainType::Road:      palIdx = 4; vShift = 0.01f; break;
				case TerrainType::Desert:    palIdx = 5; vShift = 0.0f; break;
				default:                     palIdx = 0; vShift = 0.0f; break;
			}

			float u = paletteU(palIdx);

			const uint32_t base = static_cast<uint32_t>(out.floorVerts.size());

			out.floorVerts.push_back({{x0, vShift, z0}, {0.0f, 1.0f, 0.0f}, {u, 0.0f}});
			out.floorVerts.push_back({{x1, vShift, z0}, {0.0f, 1.0f, 0.0f}, {u, 1.0f}});
			out.floorVerts.push_back({{x1, vShift, z1}, {0.0f, 1.0f, 0.0f}, {u, 2.0f}});
			out.floorVerts.push_back({{x0, vShift, z1}, {0.0f, 1.0f, 0.0f}, {u, 3.0f}});

			out.floorIndices.push_back(base + 0);
			out.floorIndices.push_back(base + 2);
			out.floorIndices.push_back(base + 1);
			out.floorIndices.push_back(base + 0);
			out.floorIndices.push_back(base + 3);
			out.floorIndices.push_back(base + 2);
		}
	}
}

void OverworldRenderer::buildWallGeometry(const Overworld& overworld, OverworldGeometry& out)
{
	float wallU = paletteU(2); // Mountain palette index

	for (int32_t r = 0; r < OVERWORLD_SIZE; ++r)
	{
		for (int32_t c = 0; c < OVERWORLD_SIZE; ++c)
		{
			const OverworldCell& cell = overworld.GetCell(r, c);
			if (cell.terrain != TerrainType::Mountain)
			{
				continue;
			}

			const float x0 = static_cast<float>(c);
			const float x1 = static_cast<float>(c + 1);
			const float z0 = static_cast<float>(r);
			const float z1 = static_cast<float>(r + 1);
			const float wallHeight = 2.0f;

			const auto isNotMountain = [&](int32_t nr, int32_t nc) -> bool
			{
				if (!Overworld::InBounds(nr, nc)) return true;
				return overworld.GetCell(nr, nc).terrain != TerrainType::Mountain;
			};

			// North face
			if (isNotMountain(r - 1, c))
			{
				const uint32_t base = static_cast<uint32_t>(out.wallVerts.size());
				out.wallVerts.push_back({{x0, 0.0f, z0}, {0.0f, 0.0f, -1.0f}, {wallU, 0.0f}});
				out.wallVerts.push_back({{x1, 0.0f, z0}, {0.0f, 0.0f, -1.0f}, {wallU, 1.0f}});
				out.wallVerts.push_back({{x1, wallHeight, z0}, {0.0f, 0.0f, -1.0f}, {wallU, 2.0f}});
				out.wallVerts.push_back({{x0, wallHeight, z0}, {0.0f, 0.0f, -1.0f}, {wallU, 3.0f}});
				out.wallIndices.push_back(base + 0);
				out.wallIndices.push_back(base + 2);
				out.wallIndices.push_back(base + 1);
				out.wallIndices.push_back(base + 0);
				out.wallIndices.push_back(base + 3);
				out.wallIndices.push_back(base + 2);
			}

			// South face
			if (isNotMountain(r + 1, c))
			{
				const uint32_t base = static_cast<uint32_t>(out.wallVerts.size());
				out.wallVerts.push_back({{x1, 0.0f, z1}, {0.0f, 0.0f, 1.0f}, {wallU, 0.0f}});
				out.wallVerts.push_back({{x0, 0.0f, z1}, {0.0f, 0.0f, 1.0f}, {wallU, 1.0f}});
				out.wallVerts.push_back({{x0, wallHeight, z1}, {0.0f, 0.0f, 1.0f}, {wallU, 2.0f}});
				out.wallVerts.push_back({{x1, wallHeight, z1}, {0.0f, 0.0f, 1.0f}, {wallU, 3.0f}});
				out.wallIndices.push_back(base + 0);
				out.wallIndices.push_back(base + 2);
				out.wallIndices.push_back(base + 1);
				out.wallIndices.push_back(base + 0);
				out.wallIndices.push_back(base + 3);
				out.wallIndices.push_back(base + 2);
			}

			// West face
			if (isNotMountain(r, c - 1))
			{
				const uint32_t base = static_cast<uint32_t>(out.wallVerts.size());
				out.wallVerts.push_back({{x0, 0.0f, z1}, {-1.0f, 0.0f, 0.0f}, {wallU, 0.0f}});
				out.wallVerts.push_back({{x0, 0.0f, z0}, {-1.0f, 0.0f, 0.0f}, {wallU, 1.0f}});
				out.wallVerts.push_back({{x0, wallHeight, z0}, {-1.0f, 0.0f, 0.0f}, {wallU, 2.0f}});
				out.wallVerts.push_back({{x0, wallHeight, z1}, {-1.0f, 0.0f, 0.0f}, {wallU, 3.0f}});
				out.wallIndices.push_back(base + 0);
				out.wallIndices.push_back(base + 2);
				out.wallIndices.push_back(base + 1);
				out.wallIndices.push_back(base + 0);
				out.wallIndices.push_back(base + 3);
				out.wallIndices.push_back(base + 2);
			}

			// East face
			if (isNotMountain(r, c + 1))
			{
				const uint32_t base = static_cast<uint32_t>(out.wallVerts.size());
				out.wallVerts.push_back({{x1, 0.0f, z0}, {1.0f, 0.0f, 0.0f}, {wallU, 0.0f}});
				out.wallVerts.push_back({{x1, 0.0f, z1}, {1.0f, 0.0f, 0.0f}, {wallU, 1.0f}});
				out.wallVerts.push_back({{x1, wallHeight, z1}, {1.0f, 0.0f, 0.0f}, {wallU, 2.0f}});
				out.wallVerts.push_back({{x1, wallHeight, z0}, {1.0f, 0.0f, 0.0f}, {wallU, 3.0f}});
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

void OverworldRenderer::buildLocationGeometry(const Overworld& overworld, OverworldGeometry& out)
{
	float locU = paletteU(6); // Gold palette index

	for (const auto& loc : overworld.Locations())
	{
		const float cx = static_cast<float>(loc.position.col) + 0.5f;
		const float cz = static_cast<float>(loc.position.row) + 0.5f;
		const float halfW = 0.3f;
		const float h = 0.8f;

		// Billboard quad (faces camera, but for now just a fixed north-south quad)
		const uint32_t base = static_cast<uint32_t>(out.locationVerts.size());

		out.locationVerts.push_back({{cx - halfW, 0.0f, cz}, {0.0f, 0.0f, -1.0f}, {locU, 0.0f}});
		out.locationVerts.push_back({{cx + halfW, 0.0f, cz}, {0.0f, 0.0f, -1.0f}, {locU, 1.0f}});
		out.locationVerts.push_back({{cx + halfW, h, cz}, {0.0f, 0.0f, -1.0f}, {locU, 2.0f}});
		out.locationVerts.push_back({{cx - halfW, h, cz}, {0.0f, 0.0f, -1.0f}, {locU, 3.0f}});

		out.locationIndices.push_back(base + 0);
		out.locationIndices.push_back(base + 2);
		out.locationIndices.push_back(base + 1);
		out.locationIndices.push_back(base + 0);
		out.locationIndices.push_back(base + 3);
		out.locationIndices.push_back(base + 2);
	}
}
