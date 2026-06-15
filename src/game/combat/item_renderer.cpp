#include "stdafx.h"
#include "game/combat/item_renderer.h"
#include "engine/renderer/camera.h"
#include "engine/renderer/renderer.h"
#include "engine/renderer/material.h"

ItemRenderer::~ItemRenderer() noexcept = default;

void ItemRenderer::Init()
{
	if (m_initialized)
	{
		return;
	}

	std::array<Vertex, 4> verts;
	constexpr float HALF_SIZE = 0.15f;
	verts[0].position = glm::vec3(-HALF_SIZE, -HALF_SIZE, 0.0f);
	verts[0].normal   = glm::vec3(0.0f, 0.0f, 1.0f);
	verts[0].texCoord = glm::vec2(0.0f, 1.0f);

	verts[1].position = glm::vec3(HALF_SIZE, -HALF_SIZE, 0.0f);
	verts[1].normal   = glm::vec3(0.0f, 0.0f, 1.0f);
	verts[1].texCoord = glm::vec2(1.0f, 1.0f);

	verts[2].position = glm::vec3(-HALF_SIZE, HALF_SIZE, 0.0f);
	verts[2].normal   = glm::vec3(0.0f, 0.0f, 1.0f);
	verts[2].texCoord = glm::vec2(0.0f, 0.0f);

	verts[3].position = glm::vec3(HALF_SIZE, HALF_SIZE, 0.0f);
	verts[3].normal   = glm::vec3(0.0f, 0.0f, 1.0f);
	verts[3].texCoord = glm::vec2(1.0f, 0.0f);

	std::array<uint32_t, 6> idx = {0, 1, 2, 1, 3, 2};

	m_billboardMesh.Create(verts, idx);
	m_initialized = true;
}

void ItemRenderer::SetMaterial(ItemType type, const Material& mat)
{
	m_materials[static_cast<size_t>(type)] = &mat;
}

void ItemRenderer::Submit(
	Renderer& renderer,
	const Camera& camera,
	const std::vector<ItemDrop>& drops
)
{
	if (!m_initialized)
	{
		return;
	}

	const glm::vec3 cameraPos = camera.Position();

	for (const ItemDrop& drop : drops)
	{
		const Material* mat = m_materials[static_cast<size_t>(drop.item.type)];
		if (!mat)
		{
			continue;
		}

		glm::vec3 worldPos = Camera::GridToWorld(drop.position);
		worldPos.y = 0.15f;

		glm::vec3 toCamera = cameraPos - worldPos;
		toCamera.y = 0.0f;
		float distSq = glm::dot(toCamera, toCamera);
		if (distSq < 0.0001f)
		{
			continue;
		}
		toCamera = glm::normalize(toCamera);

		float angle = std::atan2(toCamera.x, toCamera.z);

		glm::mat4 model(1.0f);
		model = glm::translate(model, worldPos);
		model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));

		renderer.Submit(m_billboardMesh, *mat, model);
	}
}
