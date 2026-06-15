#pragma once

#include "engine/renderer/mesh.h"
#include "engine/renderer/material.h"
#include "game/dungeon/chunk.h"

class Renderer;
class Dungeon;

struct DungeonGeometry final
{
	std::vector<Vertex> floorVerts;
	std::vector<uint32_t> floorIndices;
	std::vector<Vertex> wallVerts;
	std::vector<uint32_t> wallIndices;
	std::vector<Vertex> ceilingVerts;
	std::vector<uint32_t> ceilingIndices;
};

class DungeonRenderer final
{
public:
	void Build(const Dungeon& dungeon);
	void Submit(Renderer& renderer);

	void SetFloorMaterial(Material* mat) { m_floorMaterial = mat; }
	void SetWallMaterial(Material* mat) { m_wallMaterial = mat; }
	void SetCeilingMaterial(Material* mat) { m_ceilingMaterial = mat; }

	[[nodiscard]] bool NeedsRebuild() const noexcept { return m_dirty; }
	void SetNeedsRebuild(bool dirty) noexcept { m_dirty = dirty; }

private:
	static void buildFloorGeometry(const Chunk& chunk, DungeonGeometry& out);
	static void buildWallGeometry(const Chunk& chunk, DungeonGeometry& out);
	static void buildCeilingGeometry(const Chunk& chunk, DungeonGeometry& out);

	bool m_dirty = true;

	Mesh m_floorMesh;
	Mesh m_wallMesh;
	Mesh m_ceilingMesh;

	Material* m_floorMaterial = nullptr;
	Material* m_wallMaterial = nullptr;
	Material* m_ceilingMaterial = nullptr;
};
