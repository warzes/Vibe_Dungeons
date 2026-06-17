#include "stdafx.h"
#include "engine/resource_manager.h"

Shader* ResourceManager::GetOrCreateShader(
	std::string_view key,
	std::string_view vertexSource,
	std::string_view fragmentSource
)
{
	std::string keyStr(key);
	auto it = m_shaders.find(keyStr);
	if (it != m_shaders.end())
	{
		return it->second.get();
	}

	auto shader = std::make_unique<Shader>();
	shader->LoadFromSource(vertexSource, fragmentSource);
	Shader* ptr = shader.get();
	m_shaders[keyStr] = std::move(shader);
	return ptr;
}

Texture* ResourceManager::LoadTexture(std::string_view key, std::string_view path, bool useMipmap)
{
	std::string keyStr(key);
	auto it = m_textures.find(keyStr);
	if (it != m_textures.end())
	{
		return it->second.get();
	}

	auto tex = std::make_unique<Texture>();
	if (!tex->LoadFromFile(path, useMipmap))
	{
		return nullptr;
	}
	Texture* ptr = tex.get();
	m_textures[keyStr] = std::move(tex);
	return ptr;
}

Texture* ResourceManager::CreateCheckerboard(
	std::string_view key,
	int32_t size,
	int32_t numSquares,
	uint8_t r1, uint8_t g1, uint8_t b1, uint8_t a1,
	uint8_t r2, uint8_t g2, uint8_t b2, uint8_t a2
)
{
	std::string keyStr(key);
	auto it = m_textures.find(keyStr);
	if (it != m_textures.end())
	{
		return it->second.get();
	}

	auto tex = std::make_unique<Texture>();
	tex->CreateCheckerboard(size, numSquares, r1, g1, b1, a1, r2, g2, b2, a2);
	Texture* ptr = tex.get();
	m_textures[keyStr] = std::move(tex);
	return ptr;
}

Mesh* ResourceManager::CreateMesh(
	std::string_view key,
	std::span<const Vertex> vertices,
	std::span<const uint32_t> indices
)
{
	std::string keyStr(key);
	auto it = m_meshes.find(keyStr);
	if (it != m_meshes.end())
	{
		return it->second.get();
	}

	auto mesh = std::make_unique<Mesh>();
	mesh->Create(vertices, indices);
	Mesh* ptr = mesh.get();
	m_meshes[keyStr] = std::move(mesh);
	return ptr;
}

Material* ResourceManager::GetOrCreateMaterial(
	std::string_view key,
	Shader& shader,
	Texture& texture
)
{
	std::string keyStr(key);
	auto it = m_materials.find(keyStr);
	if (it != m_materials.end())
	{
		return it->second.get();
	}

	auto mat = std::make_unique<Material>(shader, texture);
	Material* ptr = mat.get();
	m_materials[keyStr] = std::move(mat);
	return ptr;
}

Mesh* ResourceManager::LoadMeshFromOBJ(std::string_view key, std::string_view path)
{
	std::string keyStr(key);
	auto it = m_meshes.find(keyStr);
	if (it != m_meshes.end())
	{
		return it->second.get();
	}

	auto mesh = std::make_unique<Mesh>();
	if (!mesh->LoadFromOBJ(path))
	{
		return nullptr;
	}
	Mesh* ptr = mesh.get();
	m_meshes[keyStr] = std::move(mesh);
	return ptr;
}

Mesh* ResourceManager::LoadMeshFromGLTF(std::string_view key, std::string_view path)
{
	std::string keyStr(key);
	auto it = m_meshes.find(keyStr);
	if (it != m_meshes.end())
	{
		return it->second.get();
	}

	auto mesh = std::make_unique<Mesh>();
	if (!mesh->LoadFromGLTF(path))
	{
		return nullptr;
	}
	Mesh* ptr = mesh.get();
	m_meshes[keyStr] = std::move(mesh);
	return ptr;
}

void ResourceManager::Clear() noexcept
{
	m_shaders.clear();
	m_textures.clear();
	m_meshes.clear();
	m_materials.clear();
}

void ResourceManager::CleanupUnused() noexcept
{
	// No-op: unique_ptr ownership cannot track external raw-pointer references.
	// Proper implementation requires migration to std::shared_ptr for
	// automatic use_count tracking, or explicit Acquire()/Release() ref-counting.
	// TODO: migrate to shared_ptr
}
