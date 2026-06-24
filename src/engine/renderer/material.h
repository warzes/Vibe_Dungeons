#pragma once

class Shader;
class Texture;

class Material final
{
public:
	Material() noexcept = default;
	Material(const Shader& shader, const Texture& texture) noexcept;

	void SetShader(const Shader& shader) noexcept;
	void SetTexture(const Texture& texture) noexcept;

	void Bind() const noexcept;

	[[nodiscard]] const Shader* GetShader() const noexcept
	{
		return m_shader;
	}

	[[nodiscard]] const Texture* GetTexture() const noexcept
	{
		return m_texture;
	}

private:
	const Shader* m_shader = nullptr;
	const Texture* m_texture = nullptr;
};
