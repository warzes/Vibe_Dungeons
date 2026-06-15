#pragma once

struct AABB final
{
	glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
	glm::vec3 max = glm::vec3(-std::numeric_limits<float>::max());

	[[nodiscard]] AABB Transform(const glm::mat4& matrix) const noexcept
	{
		const glm::vec3 center = (min + max) * 0.5f;
		const glm::vec3 extents = (max - min) * 0.5f;

		const glm::mat3 rotScale(matrix);
		const glm::vec3 newCenter = glm::vec3(matrix * glm::vec4(center, 1.0f));

		glm::vec3 newExtents(
			glm::dot(glm::abs(rotScale[0]), extents),
			glm::dot(glm::abs(rotScale[1]), extents),
			glm::dot(glm::abs(rotScale[2]), extents)
		);

		AABB result;
		result.min = newCenter - newExtents;
		result.max = newCenter + newExtents;
		return result;
	}
};
