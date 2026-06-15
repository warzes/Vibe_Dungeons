#pragma once

#include "engine/renderer/shader.h"
#include "engine/renderer/texture.h"
#include "engine/renderer/mesh.h"
#include "engine/renderer/material.h"

class ResourceManager final
{
public:
	ResourceManager() noexcept = default;
	~ResourceManager() noexcept = default;

	// Resources are owned by ResourceManager and returned as raw pointers.
	// Callers must ensure they do not hold pointers across Clear() calls.
	// TODO: migrate to std::shared_ptr for automatic lifetime management.

	ResourceManager(const ResourceManager&) = delete;
	ResourceManager& operator=(const ResourceManager&) = delete;
	ResourceManager(ResourceManager&&) = delete;
	ResourceManager& operator=(ResourceManager&&) = delete;

	[[nodiscard]] Shader* GetOrCreateShader(
		std::string_view key,
		std::string_view vertexSource,
		std::string_view fragmentSource
	);

	[[nodiscard]] Texture* CreateCheckerboard(
		std::string_view key,
		int32_t size,
		int32_t numSquares,
		uint8_t r1, uint8_t g1, uint8_t b1, uint8_t a1,
		uint8_t r2, uint8_t g2, uint8_t b2, uint8_t a2
	);

	[[nodiscard]] Texture* LoadTexture(std::string_view key, std::string_view path);

	[[nodiscard]] Mesh* CreateMesh(
		std::string_view key,
		std::span<const Vertex> vertices,
		std::span<const uint32_t> indices
	);

	[[nodiscard]] Mesh* LoadMeshFromOBJ(std::string_view key, std::string_view path);
	[[nodiscard]] Mesh* LoadMeshFromGLTF(std::string_view key, std::string_view path);

	[[nodiscard]] Material* GetOrCreateMaterial(
		std::string_view key,
		Shader& shader,
		Texture& texture
	);

	void Clear() noexcept;
	void CleanupUnused() noexcept;

private:
	std::unordered_map<std::string, std::unique_ptr<Shader>> m_shaders;
	std::unordered_map<std::string, std::unique_ptr<Texture>> m_textures;
	std::unordered_map<std::string, std::unique_ptr<Mesh>> m_meshes;
	std::unordered_map<std::string, std::unique_ptr<Material>> m_materials;
};
