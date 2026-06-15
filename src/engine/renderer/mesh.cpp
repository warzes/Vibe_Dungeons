#include "stdafx.h"
#include "engine/renderer/mesh.h"
#include "core/logger.h"

Mesh::~Mesh() noexcept
{
	if (m_vao != 0)
	{
		glDeleteVertexArrays(1, &m_vao);
	}
	if (m_vbo != 0)
	{
		glDeleteBuffers(1, &m_vbo);
	}
	if (m_ebo != 0)
	{
		glDeleteBuffers(1, &m_ebo);
	}
}

Mesh::Mesh(Mesh&& other) noexcept
	: m_vao(other.m_vao)
	, m_vbo(other.m_vbo)
	, m_ebo(other.m_ebo)
	, m_indexCount(other.m_indexCount)
	, m_localAABB(other.m_localAABB)
	, m_vertices(std::move(other.m_vertices))
	, m_indices(std::move(other.m_indices))
{
	other.m_vao = 0;
	other.m_vbo = 0;
	other.m_ebo = 0;
	other.m_indexCount = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept
{
	if (this != &other)
	{
		if (m_vao != 0) glDeleteVertexArrays(1, &m_vao);
		if (m_vbo != 0) glDeleteBuffers(1, &m_vbo);
		if (m_ebo != 0) glDeleteBuffers(1, &m_ebo);

		m_vao = other.m_vao;
		m_vbo = other.m_vbo;
		m_ebo = other.m_ebo;
		m_indexCount = other.m_indexCount;
		m_localAABB = other.m_localAABB;
		m_vertices = std::move(other.m_vertices);
		m_indices = std::move(other.m_indices);

		other.m_vao = 0;
		other.m_vbo = 0;
		other.m_ebo = 0;
		other.m_indexCount = 0;
	}
	return *this;
}

void Mesh::Create(std::span<const Vertex> vertices, std::span<const uint32_t> indices)
{
	m_indexCount = static_cast<uint32_t>(indices.size());

	// Compute local-space AABB
	m_localAABB = AABB{};
	for (const auto& v : vertices)
	{
		m_localAABB.min = glm::min(m_localAABB.min, v.position);
		m_localAABB.max = glm::max(m_localAABB.max, v.position);
	}

	glGenVertexArrays(1, &m_vao);
	glGenBuffers(1, &m_vbo);
	glGenBuffers(1, &m_ebo);

	glBindVertexArray(m_vao);

	glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
	glBufferData(GL_ARRAY_BUFFER,
		vertices.size_bytes(),
		vertices.data(),
		GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER,
		indices.size_bytes(),
		indices.data(),
		GL_STATIC_DRAW);

	// position
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		reinterpret_cast<const void*>(offsetof(Vertex, position)));

	// normal
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		reinterpret_cast<const void*>(offsetof(Vertex, normal)));

	// texcoord
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex),
		reinterpret_cast<const void*>(offsetof(Vertex, texCoord)));

	glBindVertexArray(0);

	// Store CPU copy for wireframe extraction
	m_vertices.assign(vertices.begin(), vertices.end());
	m_indices.assign(indices.begin(), indices.end());
}

bool Mesh::LoadFromOBJ(std::string_view path)
{
	tinyobj::ObjReaderConfig cfg;
	cfg.triangulate = true;
	cfg.vertex_color = false;

	tinyobj::ObjReader reader;
	if (!reader.ParseFromFile(std::string(path), cfg))
	{
		Logger::Warn(std::string("OBJ: failed to parse: ") + reader.Error());
		return false;
	}

	const auto& attrib = reader.GetAttrib();
	const auto& shapes = reader.GetShapes();

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	struct VertKey
	{
		int32_t v, n, t;
		bool operator==(const VertKey& o) const noexcept
		{
			return v == o.v && n == o.n && t == o.t;
		}
	};
	struct VertKeyHash
	{
		size_t operator()(const VertKey& k) const noexcept
		{
			return static_cast<size_t>(k.v) ^ (static_cast<size_t>(k.n) << 10) ^ (static_cast<size_t>(k.t) << 20);
		}
	};
	std::unordered_map<VertKey, uint32_t, VertKeyHash> vertMap;

	for (const auto& shape : shapes)
	{
		for (const auto& idx : shape.mesh.indices)
		{
			VertKey key{ idx.vertex_index, idx.normal_index, idx.texcoord_index };

			auto it = vertMap.find(key);
			if (it != vertMap.end())
			{
				indices.push_back(it->second);
				continue;
			}

			Vertex v{};

			if (idx.vertex_index >= 0)
			{
				v.position.x = static_cast<float>(attrib.vertices[3 * size_t(idx.vertex_index) + 0]);
				v.position.y = static_cast<float>(attrib.vertices[3 * size_t(idx.vertex_index) + 1]);
				v.position.z = static_cast<float>(attrib.vertices[3 * size_t(idx.vertex_index) + 2]);
			}

			if (idx.normal_index >= 0)
			{
				v.normal.x = static_cast<float>(attrib.normals[3 * size_t(idx.normal_index) + 0]);
				v.normal.y = static_cast<float>(attrib.normals[3 * size_t(idx.normal_index) + 1]);
				v.normal.z = static_cast<float>(attrib.normals[3 * size_t(idx.normal_index) + 2]);
			}

			if (idx.texcoord_index >= 0)
			{
				v.texCoord.x = static_cast<float>(attrib.texcoords[2 * size_t(idx.texcoord_index) + 0]);
				v.texCoord.y = static_cast<float>(attrib.texcoords[2 * size_t(idx.texcoord_index) + 1]);
			}

			uint32_t newIdx = static_cast<uint32_t>(vertices.size());
			vertMap[key] = newIdx;
			vertices.push_back(v);
			indices.push_back(newIdx);
		}
	}

	if (vertices.empty() || indices.empty())
	{
		Logger::Warn(std::string("OBJ: no geometry in: ") + path.data());
		return false;
	}

	Create(vertices, indices);
	Logger::Info(std::string("OBJ loaded: ") + path.data());
	return true;
}

bool Mesh::LoadFromGLTF(std::string_view path)
{
	cgltf_options options{};
	cgltf_data* data = nullptr;
	cgltf_result result = cgltf_parse_file(&options, path.data(), &data);
	if (result != cgltf_result_success)
	{
		Logger::Warn(std::string("GLTF: failed to parse: ") + path.data());
		return false;
	}

	result = cgltf_load_buffers(&options, data, path.data());
	if (result != cgltf_result_success)
	{
		cgltf_free(data);
		Logger::Warn(std::string("GLTF: failed to load buffers: ") + path.data());
		return false;
	}

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	for (size_t m = 0; m < data->meshes_count; ++m)
	{
		const auto& mesh = data->meshes[m];
		for (size_t p = 0; p < mesh.primitives_count; ++p)
		{
			const auto& prim = mesh.primitives[p];

			auto getAccessor = [&](cgltf_attribute_type type, int index) -> const cgltf_accessor*
			{
				for (size_t a = 0; a < prim.attributes_count; ++a)
				{
					if (prim.attributes[a].type == type && prim.attributes[a].index == index)
					{
						return prim.attributes[a].data;
					}
				}
				return nullptr;
			};

			const cgltf_accessor* posAcc = getAccessor(cgltf_attribute_type_position, 0);
			const cgltf_accessor* nrmAcc = getAccessor(cgltf_attribute_type_normal, 0);
			const cgltf_accessor* texAcc = getAccessor(cgltf_attribute_type_texcoord, 0);

			if (!posAcc)
			{
				continue;
			}

			size_t vertCount = posAcc->count;
			std::vector<glm::vec3> positions(vertCount);
			std::vector<glm::vec3> normals(vertCount, glm::vec3(0.0f));
			std::vector<glm::vec2> texCoords(vertCount, glm::vec2(0.0f));

			cgltf_accessor_unpack_floats(posAcc, &positions[0].x, int32_t(vertCount * 3));
			if (nrmAcc)
			{
				cgltf_accessor_unpack_floats(nrmAcc, &normals[0].x, int32_t(vertCount * 3));
			}
			if (texAcc)
			{
				cgltf_accessor_unpack_floats(texAcc, &texCoords[0].x, int32_t(vertCount * 2));
			}

			size_t baseIdx = vertices.size();
			for (size_t i = 0; i < vertCount; ++i)
			{
				vertices.push_back(Vertex{
					positions[i],
					normals[i],
					texCoords[i]
				});
			}

			if (prim.indices)
			{
				size_t idxCount = prim.indices->count;
				indices.reserve(indices.size() + idxCount);
				for (size_t i = 0; i < idxCount; ++i)
				{
					uint32_t idx = 0;
					cgltf_accessor_read_uint(prim.indices, i, &idx, 1);
					indices.push_back(static_cast<uint32_t>(baseIdx) + idx);
				}
			}
		}
	}

	cgltf_free(data);

	if (vertices.empty() || indices.empty())
	{
		Logger::Warn(std::string("GLTF: no geometry in: ") + path.data());
		return false;
	}

	Create(vertices, indices);
	Logger::Info(std::string("GLTF loaded: ") + path.data());
	return true;
}

void Mesh::Bind() const noexcept
{
	glBindVertexArray(m_vao);
}

void Mesh::Unbind() noexcept
{
	glBindVertexArray(0);
}
