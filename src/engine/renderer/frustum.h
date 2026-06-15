#pragma once

#include "core/aabb.h"

class Frustum final
{
public:
	Frustum() noexcept = default;

	void Extract(const glm::mat4& vp) noexcept;

	[[nodiscard]] bool TestAABB(const AABB& aabb) const noexcept;

private:
	struct Plane
	{
		glm::vec3 normal{};
		float distance = 0.0f;
	};

	Plane m_planes[6]{};
};
