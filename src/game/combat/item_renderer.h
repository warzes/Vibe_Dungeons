#pragma once

#include "engine/renderer/mesh.h"
#include "game/combat/item.h"

class Camera;
class Renderer;
class Material;

class ItemRenderer final
{
public:
	~ItemRenderer() noexcept;

	void Init();

	void SetMaterial(ItemType type, const Material& mat);

	void Submit(
		Renderer& renderer,
		const Camera& camera,
		const std::vector<ItemDrop>& drops
	);

private:
	Mesh m_billboardMesh;
	std::array<const Material*, static_cast<size_t>(ItemType::ITEM_TYPE_COUNT)> m_materials{};
	bool m_initialized = false;
};
