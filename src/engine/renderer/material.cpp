#include "stdafx.h"
#include "engine/renderer/material.h"
#include "engine/renderer/shader.h"
#include "engine/renderer/texture.h"

Material::Material(const Shader& shader, const Texture& texture) noexcept
	: m_shader(&shader)
	, m_texture(&texture)
{}

void Material::SetShader(const Shader& shader) noexcept
{
	m_shader = &shader;
}

void Material::SetTexture(const Texture& texture) noexcept
{
	m_texture = &texture;
}

void Material::Bind() const noexcept
{
	const Shader& shader = GetShader();
	shader.Bind();

	GetTexture().Bind(0);
	shader.SetUniform("uTexture", 0);
}
