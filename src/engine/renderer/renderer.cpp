#include "stdafx.h"
#include "engine/renderer/renderer.h"
#include "engine/renderer/mesh.h"
#include "engine/renderer/material.h"
#include "engine/renderer/shader.h"
#include "engine/renderer/texture.h"

// ── Renderer implementation ──────────────────────────────────────────────

Renderer::Renderer() noexcept
{
	glGenBuffers(1, &m_instanceVBO);

	glGenBuffers(1, &m_viewProjUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, m_viewProjUBO);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 2, nullptr, GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_viewProjUBO);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

Renderer::~Renderer() noexcept
{
	if (m_instanceVBO != 0)
	{
		glDeleteBuffers(1, &m_instanceVBO);
	}
	if (m_viewProjUBO != 0)
	{
		glDeleteBuffers(1, &m_viewProjUBO);
	}
}

uint32_t Renderer::getMaterialId(const Material* mat) noexcept
{
	auto it = m_materialIds.find(mat);
	if (it != m_materialIds.end())
	{
		return it->second;
	}
	const uint32_t id = m_materialIdCounter++;
	m_materialIds[mat] = id;
	return id;
}

uint32_t Renderer::getMeshId(const Mesh* mesh) noexcept
{
	auto it = m_meshIds.find(mesh);
	if (it != m_meshIds.end())
	{
		return it->second;
	}
	const uint32_t id = m_meshIdCounter++;
	m_meshIds[mesh] = id;
	return id;
}

void Renderer::BeginFrame(const glm::mat4& view, const glm::mat4& projection) noexcept
{
	m_commands.clear();
	m_batches.clear();
	m_instanceData.clear();
	m_viewProj = projection * view;
	m_frustum.Extract(m_viewProj);
	m_totalMeshes = 0;
	m_drawnMeshes = 0;

	// Upload view/projection to UBO once per frame
	glBindBuffer(GL_UNIFORM_BUFFER, m_viewProjUBO);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(glm::mat4), glm::value_ptr(view));
	glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::mat4), sizeof(glm::mat4), glm::value_ptr(projection));
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void Renderer::Submit(const Mesh& mesh, const Material& material, const glm::mat4& transform) noexcept
{
	m_totalMeshes++;

	const AABB worldAABB = mesh.LocalAABB().Transform(transform);
	if (!m_frustum.TestAABB(worldAABB))
	{
		return;
	}

	m_drawnMeshes++;
	m_commands.push_back({&mesh, &material, transform});
}

void Renderer::EndFrame() noexcept
{
	if (m_commands.empty())
	{
		return;
	}

	// Sort by (material, mesh) via combined uint64_t key (single comparison)
	std::sort(m_commands.begin(), m_commands.end(),
		[this](const DrawCommand& a, const DrawCommand& b) noexcept
		{
			const uint64_t keyA = (static_cast<uint64_t>(getMaterialId(a.material)) << 32)
				| getMeshId(a.mesh);
			const uint64_t keyB = (static_cast<uint64_t>(getMaterialId(b.material)) << 32)
				| getMeshId(b.mesh);
			return keyA < keyB;
		});

	// First pass: build batch ranges and collect all instance transforms
	m_instanceData.reserve(m_commands.size());

	size_t i = 0;
	while (i < m_commands.size())
	{
		const auto& first = m_commands[i];
		assert(first.mesh != nullptr && "DrawCommand with null mesh");
		assert(first.material != nullptr && "DrawCommand with null material");

		size_t batchEnd = i + 1;
		while (batchEnd < m_commands.size()
			&& m_commands[batchEnd].material == first.material
			&& m_commands[batchEnd].mesh == first.mesh)
		{
			++batchEnd;
		}

		const size_t instanceCount = batchEnd - i;
		const size_t instanceOffset = m_instanceData.size();

		for (size_t j = i; j < batchEnd; ++j)
		{
			m_instanceData.push_back(m_commands[j].transform);
		}

		m_batches.push_back({first.mesh, first.material, instanceOffset, instanceCount});
		i = batchEnd;
	}

	// Upload instance transforms — use glBufferSubData if size matches
	const GLsizeiptr instanceBytes =
		static_cast<GLsizeiptr>(m_instanceData.size() * sizeof(glm::mat4));
	glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);

	if (instanceBytes > m_instanceBufferCapacity)
	{
		glBufferData(GL_ARRAY_BUFFER, instanceBytes,
			m_instanceData.data(), GL_DYNAMIC_DRAW);
		m_instanceBufferCapacity = instanceBytes;
	}
	else
	{
		glBufferSubData(GL_ARRAY_BUFFER, 0, instanceBytes,
			m_instanceData.data());
	}

	// Second pass: draw each batch with correct instance offset
	m_lastShader = 0;
	for (const auto& batch : m_batches)
	{
		batch.material->Bind();
		{	// Bind ViewProjection UBO block to binding point 0 (once per shader)
			const GLuint shaderHandle = batch.material->GetShader().Handle();
			if (shaderHandle != m_lastShader)
			{
				const GLuint blockIdx = glGetUniformBlockIndex(shaderHandle, "ViewProjection");
				if (blockIdx != GL_INVALID_INDEX)
				{
					glUniformBlockBinding(shaderHandle, blockIdx, 0);
				}
				m_lastShader = shaderHandle;
			}
		}
		batch.mesh->Bind();

		const size_t baseBytes = batch.instanceOffset * sizeof(glm::mat4);

		// Set up per-instance attribute pointers with buffer offset
		for (int32_t j = 0; j < 4; ++j)
		{
			const uint32_t loc = 3 + static_cast<uint32_t>(j);
			glEnableVertexAttribArray(loc);
			glVertexAttribPointer(loc, 4, GL_FLOAT, GL_FALSE,
				sizeof(glm::mat4),
				reinterpret_cast<const void*>(baseBytes + static_cast<size_t>(j) * sizeof(glm::vec4)));
			glVertexAttribDivisor(loc, 1);
		}

		glDrawElementsInstanced(GL_TRIANGLES,
			static_cast<int32_t>(batch.mesh->IndexCount()),
			GL_UNSIGNED_INT, nullptr,
			static_cast<int32_t>(batch.instanceCount));

		for (int32_t j = 0; j < 4; ++j)
		{
			const uint32_t loc = 3 + static_cast<uint32_t>(j);
			glDisableVertexAttribArray(loc);
			glVertexAttribDivisor(loc, 0);
		}

		batch.mesh->Unbind();
		Shader::Unbind();
		Texture::Unbind();
	}

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	m_commands.clear();
	m_batches.clear();
	m_instanceData.clear();
}
