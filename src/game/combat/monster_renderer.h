#pragma once

#include <unordered_map>
#include "engine/renderer/mesh.h"
#include "engine/renderer/material.h"
#include "game/combat/monster.h"

class Camera;
class Renderer;

class MonsterRenderer final
{
public:
	~MonsterRenderer() noexcept;

	void Init();

	void Submit(
		Renderer& renderer,
		const Camera& camera,
		const std::vector<Monster>& monsters,
		const std::unordered_map<std::string, Material*>& materialsByType
	);

private:
	Mesh m_billboardMesh;
	bool m_initialized = false;
};
