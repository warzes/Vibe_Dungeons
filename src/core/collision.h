#pragma once

#include "core/aabb.h"

struct Sphere final
{
	glm::vec3 center = glm::vec3(0.0f);
	float radius = 1.0f;
};

namespace Collision
{

[[nodiscard]] inline bool AABBvsAABB(const AABB& a, const AABB& b) noexcept
{
	return (a.min.x <= b.max.x && a.max.x >= b.min.x)
		&& (a.min.y <= b.max.y && a.max.y >= b.min.y)
		&& (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

[[nodiscard]] inline bool SphereVsSphere(const Sphere& a, const Sphere& b) noexcept
{
		const glm::vec3 diff = a.center - b.center;
		const float distSq = glm::dot(diff, diff);
		const float rSum = a.radius + b.radius;
	return distSq <= rSum * rSum;
}

[[nodiscard]] inline bool SphereVsAABB(const Sphere& sphere, const AABB& aabb) noexcept
{
	glm::vec3 closest;
	closest.x = std::max(aabb.min.x, std::min(sphere.center.x, aabb.max.x));
	closest.y = std::max(aabb.min.y, std::min(sphere.center.y, aabb.max.y));
	closest.z = std::max(aabb.min.z, std::min(sphere.center.z, aabb.max.z));

	const glm::vec3 diff = sphere.center - closest;
	const float distSq = glm::dot(diff, diff);
	return distSq <= sphere.radius * sphere.radius;
}

[[nodiscard]] inline bool RayVsAABB(
	glm::vec3 origin,
	glm::vec3 dir,
	const AABB& aabb,
	float& tMin,
	float& tMax
) noexcept
{
	const glm::vec3 invDir = 1.0f / dir;

	float t1 = (aabb.min.x - origin.x) * invDir.x;
	float t2 = (aabb.max.x - origin.x) * invDir.x;
	float t3 = (aabb.min.y - origin.y) * invDir.y;
	float t4 = (aabb.max.y - origin.y) * invDir.y;
	float t5 = (aabb.min.z - origin.z) * invDir.z;
	float t6 = (aabb.max.z - origin.z) * invDir.z;

	tMin = std::max(std::max(std::min(t1, t2), std::min(t3, t4)), std::min(t5, t6));
	tMax = std::min(std::min(std::max(t1, t2), std::max(t3, t4)), std::max(t5, t6));

	return tMax >= 0.0f && tMin <= tMax;
}

[[nodiscard]] inline bool RayVsSphere(
	glm::vec3 origin,
	glm::vec3 dir,
	const Sphere& sphere,
	float& t
) noexcept
{
	const glm::vec3 oc = origin - sphere.center;
	const float a = glm::dot(dir, dir);
	const float b = 2.0f * glm::dot(oc, dir);
	const float c = glm::dot(oc, oc) - sphere.radius * sphere.radius;
	const float disc = b * b - 4.0f * a * c;

	if (disc < 0.0f)
	{
		return false;
	}

	const float discSqrt = std::sqrt(disc);
	const float t0 = (-b - discSqrt) / (2.0f * a);
	const float t1 = (-b + discSqrt) / (2.0f * a);

	t = (t0 < t1) ? t0 : t1;
	return true;
}

} // namespace Collision
