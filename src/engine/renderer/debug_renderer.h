#pragma once

#include "engine/renderer/shader.h"
#include "engine/renderer/mesh.h"
#include "engine/renderer/frustum.h"
#include "core/collision.h"

class Renderer;

class DebugRenderer final
{
public:
	DebugRenderer() noexcept;
	~DebugRenderer() noexcept;

	DebugRenderer(const DebugRenderer&) = delete;
	DebugRenderer& operator=(const DebugRenderer&) = delete;

	void Init() noexcept;

	void DrawAABB(const AABB& aabb, const glm::vec3& color = glm::vec3(0.0f, 1.0f, 0.0f)) noexcept;
	void DrawAABB(const AABB& localAABB, const glm::mat4& worldMatrix, const glm::vec3& color = glm::vec3(0.0f, 1.0f, 0.0f)) noexcept;
	void DrawSphere(const Sphere& sphere, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 0.0f)) noexcept;
	void DrawFrustum(const glm::mat4& viewProj, const glm::vec3& color = glm::vec3(0.0f, 1.0f, 1.0f)) noexcept;
	void DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color = glm::vec3(1.0f, 1.0f, 1.0f)) noexcept;

	void DrawMeshWireframe(
		std::span<const Vertex> vertices,
		std::span<const uint32_t> indices,
		const glm::mat4& worldMatrix,
		const glm::vec3& color = glm::vec3(0.0f, 0.5f, 1.0f)
	) noexcept;

	void SetViewProj(const glm::mat4& viewProj) noexcept;
	void Flush(const glm::mat4& view, const glm::mat4& projection) noexcept;

	void Clear() noexcept;

	void SetEnabled(bool enabled) noexcept { m_enabled = enabled; }
	[[nodiscard]] bool IsEnabled() const noexcept { return m_enabled; }

private:
	struct DebugLine
	{
		glm::vec3 from;
		glm::vec3 to;
		glm::vec3 color;
	};

	bool m_enabled = false;
	bool m_initialized = false;

	Shader m_shader;
	uint32_t m_vao = 0;
	uint32_t m_vbo = 0;
	Frustum m_frustum;

	std::vector<DebugLine> m_lines;
	std::vector<float> m_vertexData;

	static constexpr int32_t SPHERE_SEGMENTS = 16;
};
