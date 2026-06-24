#include "stdafx.h"
#include "engine/renderer/debug_renderer.h"
#include "engine/renderer/shader_sources.h"
#include "core/logger.h"

DebugRenderer::DebugRenderer() noexcept = default;

DebugRenderer::~DebugRenderer() noexcept
{
	if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
	if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
}

void DebugRenderer::Init() noexcept
{
	try
	{
		m_shader.LoadFromSource(DEBUG_VERT_SRC, DEBUG_FRAG_SRC);

		glGenVertexArrays(1, &m_vao);
		glGenBuffers(1, &m_vbo);

		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

		// vec3 aPos (location 0)
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), nullptr);
		glEnableVertexAttribArray(0);
		// vec3 aColor (location 1)
		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float),
			reinterpret_cast<const void*>(3 * sizeof(float)));
		glEnableVertexAttribArray(1);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		m_initialized = true;
		Logger::Info("DebugRenderer initialized");
	}
	catch (const std::exception& e)
	{
		Logger::Warn(std::string("DebugRenderer init failed: ") + e.what());
	}
}

void DebugRenderer::SetViewProj(const glm::mat4& viewProj) noexcept
{
	m_frustum.Extract(viewProj);
}

void DebugRenderer::DrawAABB(const AABB& aabb, const glm::vec3& color) noexcept
{
	if (!m_enabled) return;

	const auto& min = aabb.min;
	const auto& max = aabb.max;

	glm::vec3 corners[8];
	corners[0] = glm::vec3(min.x, min.y, min.z);
	corners[1] = glm::vec3(max.x, min.y, min.z);
	corners[2] = glm::vec3(max.x, max.y, min.z);
	corners[3] = glm::vec3(min.x, max.y, min.z);
	corners[4] = glm::vec3(min.x, min.y, max.z);
	corners[5] = glm::vec3(max.x, min.y, max.z);
	corners[6] = glm::vec3(max.x, max.y, max.z);
	corners[7] = glm::vec3(min.x, max.y, max.z);

	constexpr int EDGES[24] = {
		0,1, 1,2, 2,3, 3,0,
		4,5, 5,6, 6,7, 7,4,
		0,4, 1,5, 2,6, 3,7
	};

	for (int i = 0; i < 24; i += 2)
	{
		DrawLine(corners[EDGES[i]], corners[EDGES[i + 1]], color);
	}
}

void DebugRenderer::DrawAABB(const AABB& localAABB, const glm::mat4& worldMatrix, const glm::vec3& color) noexcept
{
	if (!m_enabled) return;

	const AABB worldAABB = localAABB.Transform(worldMatrix);
	const auto& min = worldAABB.min;
	const auto& max = worldAABB.max;

	glm::vec3 corners[8];
	corners[0] = glm::vec3(min.x, min.y, min.z);
	corners[1] = glm::vec3(max.x, min.y, min.z);
	corners[2] = glm::vec3(max.x, max.y, min.z);
	corners[3] = glm::vec3(min.x, max.y, min.z);
	corners[4] = glm::vec3(min.x, min.y, max.z);
	corners[5] = glm::vec3(max.x, min.y, max.z);
	corners[6] = glm::vec3(max.x, max.y, max.z);
	corners[7] = glm::vec3(min.x, max.y, max.z);

	constexpr int EDGES[24] = {
		0,1, 1,2, 2,3, 3,0,
		4,5, 5,6, 6,7, 7,4,
		0,4, 1,5, 2,6, 3,7
	};

	for (int i = 0; i < 24; i += 2)
	{
		DrawLine(corners[EDGES[i]], corners[EDGES[i + 1]], color);
	}
}

void DebugRenderer::DrawSphere(const Sphere& sphere, const glm::vec3& color) noexcept
{
	if (!m_enabled) return;

	const glm::vec3& c = sphere.center;
	const float r = sphere.radius;

	// Ring in XY plane
	for (int i = 0; i < SPHERE_SEGMENTS; ++i)
	{
		const float a1 = 2.0f * 3.14159265f * static_cast<float>(i) / static_cast<float>(SPHERE_SEGMENTS);
		const float a2 = 2.0f * 3.14159265f * static_cast<float>(i + 1) / static_cast<float>(SPHERE_SEGMENTS);
		DrawLine(
			c + glm::vec3(std::cos(a1) * r, std::sin(a1) * r, 0.0f),
			c + glm::vec3(std::cos(a2) * r, std::sin(a2) * r, 0.0f),
			color);
		DrawLine(
			c + glm::vec3(std::cos(a1) * r, 0.0f, std::sin(a1) * r),
			c + glm::vec3(std::cos(a2) * r, 0.0f, std::sin(a2) * r),
			color);
		DrawLine(
			c + glm::vec3(0.0f, std::cos(a1) * r, std::sin(a1) * r),
			c + glm::vec3(0.0f, std::cos(a2) * r, std::sin(a2) * r),
			color);
	}
}

void DebugRenderer::DrawFrustum(const glm::mat4& viewProj, const glm::vec3& color) noexcept
{
	if (!m_enabled) return;

	const glm::mat4 invVP = glm::inverse(viewProj);

	// NDC corners for near (z=-1) and far (z=1) planes
	const glm::vec4 ndcCorners[8] = {
		{-1, -1, -1, 1}, { 1, -1, -1, 1}, { 1,  1, -1, 1}, {-1,  1, -1, 1},
		{-1, -1,  1, 1}, { 1, -1,  1, 1}, { 1,  1,  1, 1}, {-1,  1,  1, 1},
	};

	glm::vec3 world[8];
	for (int i = 0; i < 8; ++i)
	{
		glm::vec4 p = invVP * ndcCorners[i];
		world[i] = glm::vec3(p) / p.w;
	}

	// Near plane edges
	for (int i = 0; i < 4; ++i)
	{
		DrawLine(world[i], world[(i + 1) % 4], color);
	}
	// Far plane edges
	for (int i = 4; i < 8; ++i)
	{
		DrawLine(world[i], world[((i + 1) % 4) + 4], color);
	}
	// Connecting edges
	for (int i = 0; i < 4; ++i)
	{
		DrawLine(world[i], world[i + 4], color);
	}
}

void DebugRenderer::DrawOverlayLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) noexcept
{
	m_overlayLines.push_back({ from, to, color });
}

void DebugRenderer::ClearOverlay() noexcept
{
	m_overlayLines.clear();
}

void DebugRenderer::DrawLine(const glm::vec3& from, const glm::vec3& to, const glm::vec3& color) noexcept
{
	if (!m_enabled) return;
	m_lines.push_back({ from, to, color });
}

void DebugRenderer::DrawMeshWireframe(
	std::span<const Vertex> vertices,
	std::span<const uint32_t> indices,
	const glm::mat4& worldMatrix,
	const glm::vec3& color
) noexcept
{
	if (!m_enabled)
	{
		return;
	}

	if (indices.size() % 3 != 0)
	{
		return;
	}

	// Compute world-space AABB for frustum culling
	AABB worldAABB;
	bool first = true;
	for (const auto& vert : vertices)
	{
		const glm::vec4 t = worldMatrix * glm::vec4(vert.position, 1.0f);
		const glm::vec3 wp = glm::vec3(t) / t.w;
		if (first)
		{
			worldAABB.min = worldAABB.max = wp;
			first = false;
		}
		else
		{
			worldAABB.min = glm::min(worldAABB.min, wp);
			worldAABB.max = glm::max(worldAABB.max, wp);
		}
	}

	if (!m_frustum.TestAABB(worldAABB))
	{
		return; // Culled
	}

	auto xform = [&](const glm::vec3& p) -> glm::vec3
	{
		const glm::vec4 t = worldMatrix * glm::vec4(p, 1.0f);
		return glm::vec3(t) / t.w;
	};

	for (size_t i = 0; i < indices.size(); i += 3)
	{
		const glm::vec3 v0 = xform(vertices[indices[i + 0]].position);
		const glm::vec3 v1 = xform(vertices[indices[i + 1]].position);
		const glm::vec3 v2 = xform(vertices[indices[i + 2]].position);

		DrawLine(v0, v1, color);
		DrawLine(v1, v2, color);
		DrawLine(v2, v0, color);
	}
}

void DebugRenderer::Flush(const glm::mat4& view, const glm::mat4& projection) noexcept
{
	if (!m_initialized)
	{
		return;
	}

	m_vertexData.clear();

	// Always render overlay lines (independent of debug mode)
	if (!m_overlayLines.empty())
	{
		m_vertexData.clear();
		m_vertexData.reserve(m_overlayLines.size() * 12);
		for (const auto& line : m_overlayLines)
		{
			m_vertexData.push_back(line.from.x);
			m_vertexData.push_back(line.from.y);
			m_vertexData.push_back(line.from.z);
			m_vertexData.push_back(line.color.r);
			m_vertexData.push_back(line.color.g);
			m_vertexData.push_back(line.color.b);

			m_vertexData.push_back(line.to.x);
			m_vertexData.push_back(line.to.y);
			m_vertexData.push_back(line.to.z);
			m_vertexData.push_back(line.color.r);
			m_vertexData.push_back(line.color.g);
			m_vertexData.push_back(line.color.b);
		}

		// Save GL state
		GLboolean wasDepthTest = glIsEnabled(GL_DEPTH_TEST);
		GLint wasVAO = 0;
		GLint wasVBO = 0;
		GLint wasProgram = 0;
		glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &wasVAO);
		glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &wasVBO);
		glGetIntegerv(GL_CURRENT_PROGRAM, &wasProgram);

		glDisable(GL_DEPTH_TEST);

		m_shader.Bind();
		m_shader.SetUniform("uViewProj", projection * view);

		glBindVertexArray(m_vao);
		glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
		glBufferData(GL_ARRAY_BUFFER,
			m_vertexData.size() * sizeof(float),
			m_vertexData.data(),
			GL_DYNAMIC_DRAW);

		glDrawArrays(GL_LINES, 0, static_cast<int32_t>(m_overlayLines.size() * 2));

		// Restore GL state
		if (wasDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
		glBindVertexArray(static_cast<GLuint>(wasVAO));
		glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(wasVBO));
		glUseProgram(static_cast<GLuint>(wasProgram));

		m_overlayLines.clear();
	}

	// Debug-only lines (only when enabled)
	if (!m_enabled || m_lines.empty())
	{
		m_lines.clear();
		return;
	}

	m_vertexData.clear();
	m_vertexData.reserve(m_lines.size() * 12);

	for (const auto& line : m_lines)
	{
		m_vertexData.push_back(line.from.x);
		m_vertexData.push_back(line.from.y);
		m_vertexData.push_back(line.from.z);
		m_vertexData.push_back(line.color.r);
		m_vertexData.push_back(line.color.g);
		m_vertexData.push_back(line.color.b);

		m_vertexData.push_back(line.to.x);
		m_vertexData.push_back(line.to.y);
		m_vertexData.push_back(line.to.z);
		m_vertexData.push_back(line.color.r);
		m_vertexData.push_back(line.color.g);
		m_vertexData.push_back(line.color.b);
	}

	// Save GL state
	GLboolean wasDepthTest = glIsEnabled(GL_DEPTH_TEST);
	GLint wasVAO = 0;
	GLint wasVBO = 0;
	GLint wasProgram = 0;
	glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &wasVAO);
	glGetIntegerv(GL_ARRAY_BUFFER_BINDING, &wasVBO);
	glGetIntegerv(GL_CURRENT_PROGRAM, &wasProgram);

	glDisable(GL_DEPTH_TEST);

	m_shader.Bind();
	m_shader.SetUniform("uViewProj", projection * view);

	glBindVertexArray(m_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		m_vertexData.size() * sizeof(float),
		m_vertexData.data(),
		GL_DYNAMIC_DRAW);

	glDrawArrays(GL_LINES, 0, static_cast<int32_t>(m_lines.size() * 2));

	// Restore GL state
	if (wasDepthTest) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
	glBindVertexArray(static_cast<GLuint>(wasVAO));
	glBindBuffer(GL_ARRAY_BUFFER, static_cast<GLuint>(wasVBO));
	glUseProgram(static_cast<GLuint>(wasProgram));

	m_lines.clear();
	m_vertexData.clear();
}

void DebugRenderer::Clear() noexcept
{
	m_lines.clear();
}
