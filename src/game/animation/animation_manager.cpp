#include "stdafx.h"
#include "game/animation/animation_manager.h"

#include "engine/resource_manager.h"
#include "engine/renderer/shader_sources.h"
#include "core/logger.h"

//=============================================================================

AnimationManager::~AnimationManager() noexcept = default;

//=============================================================================

static constexpr float PI = 3.14159265358979323846f;
static constexpr float TWO_PI = 2.0f * PI;

//=============================================================================

void AnimationManager::Init(ResourceManager& resources) noexcept
{
	if (m_initialized)
	{
		return;
	}

	m_shader = resources.GetOrCreateShader("anim_billboard", ANIM_VERT_SRC, ANIM_FRAG_SRC);
	if (!m_shader)
	{
		Logger::Error("AnimationManager: failed to create billboard shader");
		return;
	}

	std::array<Vertex, 4> verts;
	verts[0].position = glm::vec3(-0.5f, 0.0f, -0.5f);
	verts[0].normal   = glm::vec3(0.0f, 0.0f, 1.0f);
	verts[0].texCoord = glm::vec2(0.0f, 1.0f);

	verts[1].position = glm::vec3(0.5f, 0.0f, -0.5f);
	verts[1].normal   = glm::vec3(0.0f, 0.0f, 1.0f);
	verts[1].texCoord = glm::vec2(1.0f, 1.0f);

	verts[2].position = glm::vec3(-0.5f, 1.0f, -0.5f);
	verts[2].normal   = glm::vec3(0.0f, 0.0f, 1.0f);
	verts[2].texCoord = glm::vec2(0.0f, 0.0f);

	verts[3].position = glm::vec3(0.5f, 1.0f, -0.5f);
	verts[3].normal   = glm::vec3(0.0f, 0.0f, 1.0f);
	verts[3].texCoord = glm::vec2(1.0f, 0.0f);

	std::array<uint32_t, 6> idx = {0, 1, 2, 1, 3, 2};

	m_billboardMesh.Create(verts, idx);
	m_initialized = true;
}

//=============================================================================

void AnimationManager::Shutdown() noexcept
{
	m_animations.clear();
	m_shader = nullptr;
	m_initialized = false;
}

//=============================================================================

void AnimationManager::Spawn(AnimationEffectType type, const glm::vec3& worldPos, float duration, float scale) noexcept
{
	float actualScale = (scale > 0.0f) ? scale : DefaultScaleForType(type);

	ActiveAnimation anim;
	anim.type = type;
	anim.worldPos = worldPos;
	anim.duration = duration;
	anim.scale = actualScale;
	anim.elapsed = 0.0f;
	anim.finished = false;
	m_animations.push_back(std::move(anim));
}

//=============================================================================

void AnimationManager::Update(float dt) noexcept
{
	for (size_t i = 0; i < m_animations.size(); )
	{
		m_animations[i].elapsed += dt;
		if (m_animations[i].elapsed >= m_animations[i].duration)
		{
			m_animations[i].finished = true;
		}

		if (m_animations[i].finished)
		{
			m_animations[i] = m_animations.back();
			m_animations.pop_back();
		}
		else
		{
			++i;
		}
	}
}

//=============================================================================

void AnimationManager::Render(const Camera& camera) noexcept
{
	if (!m_initialized || m_animations.empty() || !m_shader)
	{
		return;
	}

	const glm::vec3 cameraPos = camera.Position();
	const glm::mat4 viewProj = camera.ProjectionMatrix() * camera.ViewMatrix();

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

	m_shader->Bind();
	m_shader->SetUniform("uViewProj", viewProj);

	m_billboardMesh.Bind();

	for (const ActiveAnimation& anim : m_animations)
	{
		const float t = anim.elapsed / anim.duration;

		glm::vec3 toCamera = cameraPos - anim.worldPos;
		if (glm::dot(toCamera, toCamera) < 0.0001f)
		{
			continue;
		}
		toCamera.y = 0.0f;
		toCamera = glm::normalize(toCamera);

		float angle = std::atan2(toCamera.x, toCamera.z);

		glm::vec4 color = ColorForType(anim.type);
		color.a *= (1.0f - t);

		float currentScale = anim.scale;

		if (anim.type == AnimationEffectType::DeathDissolve)
		{
			currentScale *= (0.5f + 0.5f * (1.0f - t));
		}
		else if (anim.type == AnimationEffectType::AttackSwing)
		{
			const float sweep = std::sin(t * PI);
			angle += sweep * 0.4f;
			currentScale *= (0.8f + 0.4f * sweep);
		}
		else
		{
			const float pulse = 1.0f + 0.3f * std::sin(t * TWO_PI * 4.0f);
			currentScale *= pulse;
		}

		glm::mat4 model(1.0f);
		model = glm::translate(model, anim.worldPos + glm::vec3(0.0f, 0.5f * currentScale, 0.0f));
		model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
		model = glm::scale(model, glm::vec3(currentScale));

		m_shader->SetUniform("uModel", model);
		m_shader->SetUniform("uColor", color);

		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_billboardMesh.IndexCount()), GL_UNSIGNED_INT, nullptr);
	}

	m_billboardMesh.Unbind();
	m_shader->Unbind();

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);
}

//=============================================================================

glm::vec4 AnimationManager::ColorForType(AnimationEffectType type) noexcept
{
	switch (type)
	{
		case AnimationEffectType::AttackSwing:
			return glm::vec4(0.95f, 0.85f, 0.40f, 0.80f);
		case AnimationEffectType::SpellFire:
			return glm::vec4(0.95f, 0.25f, 0.05f, 0.80f);
		case AnimationEffectType::SpellIce:
			return glm::vec4(0.30f, 0.60f, 0.95f, 0.80f);
		case AnimationEffectType::SpellLightning:
			return glm::vec4(0.95f, 0.95f, 0.20f, 0.80f);
		case AnimationEffectType::SpellHoly:
			return glm::vec4(0.95f, 0.95f, 0.85f, 0.80f);
		case AnimationEffectType::DeathDissolve:
			return glm::vec4(0.30f, 0.15f, 0.05f, 0.70f);
	}
	return glm::vec4(1.0f);
}

//=============================================================================

float AnimationManager::DefaultScaleForType(AnimationEffectType type) noexcept
{
	switch (type)
	{
		case AnimationEffectType::AttackSwing:
			return 1.0f;
		case AnimationEffectType::SpellFire:
		case AnimationEffectType::SpellIce:
		case AnimationEffectType::SpellLightning:
		case AnimationEffectType::SpellHoly:
			return 0.7f;
		case AnimationEffectType::DeathDissolve:
			return 0.9f;
	}
	return 1.0f;
}
