#pragma once

#include "core/aabb.h"

struct Vertex
{
	glm::vec3 position;
	glm::vec3 normal;
	glm::vec2 texCoord;
};

class Mesh final
{
public:
	Mesh() noexcept = default;
	~Mesh() noexcept;

	Mesh(const Mesh&) = delete;
	Mesh& operator=(const Mesh&) = delete;
	Mesh(Mesh&& other) noexcept;
	Mesh& operator=(Mesh&& other) noexcept;

	void Create(std::span<const Vertex> vertices, std::span<const uint32_t> indices);

	[[nodiscard]] std::span<const Vertex> GetVertices() const noexcept
	{
		return m_vertices;
	}

	[[nodiscard]] std::span<const uint32_t> GetIndices() const noexcept
	{
		return m_indices;
	}

	[[nodiscard]] bool LoadFromOBJ(std::string_view path);
	[[nodiscard]] bool LoadFromGLTF(std::string_view path);

	void Bind() const noexcept;
	static void Unbind() noexcept;

	[[nodiscard]] uint32_t IndexCount() const noexcept
	{
		return m_indexCount;
	}

	[[nodiscard]] explicit operator bool() const noexcept
	{
		return m_vao != 0;
	}

	[[nodiscard]] const AABB& LocalAABB() const noexcept
	{
		return m_localAABB;
	}

private:
	uint32_t m_vao = 0;
	uint32_t m_vbo = 0;
	uint32_t m_ebo = 0;
	uint32_t m_indexCount = 0;
	AABB m_localAABB;

	std::vector<Vertex> m_vertices;
	std::vector<uint32_t> m_indices;
};
