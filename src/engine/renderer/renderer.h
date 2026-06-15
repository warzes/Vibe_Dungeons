#pragma once

#include "engine/renderer/frustum.h"

class Mesh;
class Material;

class Renderer final
{
public:
	Renderer() noexcept;
	~Renderer() noexcept;

	Renderer(const Renderer&) = delete;
	Renderer& operator=(const Renderer&) = delete;
	Renderer(Renderer&&) = delete;
	Renderer& operator=(Renderer&&) = delete;

	void BeginFrame(const glm::mat4& view, const glm::mat4& projection) noexcept;
	void Submit(const Mesh& mesh, const Material& material, const glm::mat4& transform) noexcept;
	void EndFrame() noexcept;

	[[nodiscard]] int32_t TotalMeshes() const noexcept
	{
		return m_totalMeshes;
	}

	[[nodiscard]] int32_t DrawnMeshes() const noexcept
	{
		return m_drawnMeshes;
	}

	[[nodiscard]] const glm::mat4& ViewProj() const noexcept
	{
		return m_viewProj;
	}

private:
	struct DrawCommand
	{
		const Mesh* mesh;
		const Material* material;
		glm::mat4 transform;
	};

	struct BatchRange
	{
		const Mesh* mesh;
		const Material* material;
		size_t instanceOffset; //< index into m_instanceData
		size_t instanceCount;
	};

	glm::mat4 m_viewProj{ 1.0f };
	Frustum m_frustum;
	std::vector<DrawCommand> m_commands;
	std::vector<glm::mat4> m_instanceData;
	std::vector<BatchRange> m_batches;
	int32_t m_totalMeshes = 0;
	int32_t m_drawnMeshes = 0;
	uint32_t m_instanceVBO = 0;
	GLsizeiptr m_instanceBufferCapacity = 0;
	uint32_t m_viewProjUBO = 0;
	uint32_t m_lastShader = 0;
	uint32_t m_materialIdCounter = 0;
	uint32_t m_meshIdCounter = 0;
	std::unordered_map<const Material*, uint32_t> m_materialIds;
	std::unordered_map<const Mesh*, uint32_t> m_meshIds;

	[[nodiscard]] uint32_t getMaterialId(const Material* mat) noexcept;
	[[nodiscard]] uint32_t getMeshId(const Mesh* mesh) noexcept;
};
