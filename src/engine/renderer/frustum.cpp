#include "stdafx.h"
#include "engine/renderer/frustum.h"

void Frustum::Extract(const glm::mat4& vp) noexcept
{
	// Left   (col3 + col0)
	m_planes[0].normal.x = vp[0][3] + vp[0][0];
	m_planes[0].normal.y = vp[1][3] + vp[1][0];
	m_planes[0].normal.z = vp[2][3] + vp[2][0];
	m_planes[0].distance = vp[3][3] + vp[3][0];

	// Right  (col3 - col0)
	m_planes[1].normal.x = vp[0][3] - vp[0][0];
	m_planes[1].normal.y = vp[1][3] - vp[1][0];
	m_planes[1].normal.z = vp[2][3] - vp[2][0];
	m_planes[1].distance = vp[3][3] - vp[3][0];

	// Bottom (col3 + col1)
	m_planes[2].normal.x = vp[0][3] + vp[0][1];
	m_planes[2].normal.y = vp[1][3] + vp[1][1];
	m_planes[2].normal.z = vp[2][3] + vp[2][1];
	m_planes[2].distance = vp[3][3] + vp[3][1];

	// Top    (col3 - col1)
	m_planes[3].normal.x = vp[0][3] - vp[0][1];
	m_planes[3].normal.y = vp[1][3] - vp[1][1];
	m_planes[3].normal.z = vp[2][3] - vp[2][1];
	m_planes[3].distance = vp[3][3] - vp[3][1];

	// Near   (col3 + col2)
	m_planes[4].normal.x = vp[0][3] + vp[0][2];
	m_planes[4].normal.y = vp[1][3] + vp[1][2];
	m_planes[4].normal.z = vp[2][3] + vp[2][2];
	m_planes[4].distance = vp[3][3] + vp[3][2];

	// Far    (col3 - col2)
	m_planes[5].normal.x = vp[0][3] - vp[0][2];
	m_planes[5].normal.y = vp[1][3] - vp[1][2];
	m_planes[5].normal.z = vp[2][3] - vp[2][2];
	m_planes[5].distance = vp[3][3] - vp[3][2];

	// Normalize all planes
	for (auto& plane : m_planes)
	{
		const float len = glm::length(plane.normal);
		if (len > 0.0f)
		{
			const float invLen = 1.0f / len;
			plane.normal *= invLen;
			plane.distance *= invLen;
		}
	}
}

bool Frustum::TestAABB(const AABB& aabb) const noexcept
{
	for (const auto& plane : m_planes)
	{
		// p-vertex: corner most in the direction of the plane normal
		glm::vec3 p = aabb.min;
		if (plane.normal.x >= 0.0f) p.x = aabb.max.x;
		if (plane.normal.y >= 0.0f) p.y = aabb.max.y;
		if (plane.normal.z >= 0.0f) p.z = aabb.max.z;

		if (glm::dot(plane.normal, p) + plane.distance < 0.0f)
		{
			return false;
		}
	}
	return true;
}
