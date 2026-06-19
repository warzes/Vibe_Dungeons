#pragma once

#include "engine/renderer/mesh.h"
#include "engine/renderer/material.h"
#include "game/overworld/overworld.h"
#include "game/overworld/overworld_cell.h"

class Renderer;

struct OverworldGeometry final
{
	std::vector<Vertex> floorVerts;
	std::vector<uint32_t> floorIndices;
	std::vector<Vertex> wallVerts;
	std::vector<uint32_t> wallIndices;
	std::vector<Vertex> locationVerts;
	std::vector<uint32_t> locationIndices;
};

class OverworldRenderer final
{
public:
	void Build(const Overworld& overworld, GridPosition cameraPos = {}, int32_t viewRadius = 64);
	void Submit(Renderer& renderer);

	void SetFloorMaterial(Material* mat) { m_floorMaterial = mat; }
	void SetWallMaterial(Material* mat) { m_wallMaterial = mat; }
	void SetLocationMaterial(Material* mat) { m_locationMaterial = mat; }

	[[nodiscard]] bool NeedsRebuild() const noexcept { return m_dirty; }
	void SetNeedsRebuild(bool dirty) noexcept { m_dirty = dirty; }

private:
	static void buildFloorGeometry(
		const Overworld& overworld,
		GridPosition cameraPos, int32_t viewRadius,
		OverworldGeometry& out);
	static void buildWallGeometry(
		const Overworld& overworld,
		GridPosition cameraPos, int32_t viewRadius,
		OverworldGeometry& out);
	static void buildLocationGeometry(
		const Overworld& overworld,
		GridPosition cameraPos, int32_t viewRadius,
		OverworldGeometry& out);

	bool m_dirty = true;

	Mesh m_floorMesh;
	Mesh m_wallMesh;
	Mesh m_locationMesh;

	Material* m_floorMaterial = nullptr;
	Material* m_wallMaterial = nullptr;
	Material* m_locationMaterial = nullptr;
};
