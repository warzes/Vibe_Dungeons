#pragma once

#include "engine/renderer/shader.h"
#include "engine/renderer/mesh.h"
#include "engine/renderer/camera.h"

class Renderer;
class ResourceManager;

// ---------------------------------------------------------------------

enum class AnimationEffectType
{
	AttackSwing,
	SpellFire,
	SpellIce,
	SpellLightning,
	SpellHoly,
	DeathDissolve
};

// ---------------------------------------------------------------------

struct ActiveAnimation
{
	AnimationEffectType type;
	glm::vec3 worldPos;
	float elapsed = 0.0f;
	float duration = 0.5f;
	float scale = 1.0f;
	bool finished = false;
};

// ---------------------------------------------------------------------

class AnimationManager final
{
public:
	~AnimationManager() noexcept;

	void Init(ResourceManager& resources) noexcept;
	void Shutdown() noexcept;

	void Spawn(AnimationEffectType type, const glm::vec3& worldPos, float duration = 0.5f, float scale = 1.0f) noexcept;
	void Update(float dt) noexcept;

	void Render(const Camera& camera) noexcept;

	[[nodiscard]] int32_t ActiveCount() const noexcept
	{
		return static_cast<int32_t>(m_animations.size());
	}

private:
	static glm::vec4 ColorForType(AnimationEffectType type) noexcept;
	static float DefaultScaleForType(AnimationEffectType type) noexcept;

	Shader* m_shader = nullptr;
	Mesh m_billboardMesh;
	bool m_initialized = false;

	std::vector<ActiveAnimation> m_animations;
};
