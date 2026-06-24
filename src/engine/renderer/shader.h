#pragma once

class Shader final
{
public:
	Shader() noexcept = default;
	~Shader() noexcept;

	Shader(const Shader&) = delete;
	Shader& operator=(const Shader&) = delete;
	Shader(Shader&&) = delete;
	Shader& operator=(Shader&&) = delete;

	void LoadFromSource(
		std::string_view vertexSource,
		std::string_view fragmentSource
	);

	void Bind() const noexcept;
	static void Unbind() noexcept;

	[[nodiscard]] uint32_t Handle() const noexcept
	{
		return m_program;
	}

	[[nodiscard]] int32_t GetUniformLocation(std::string_view name) const noexcept;
	[[nodiscard]] uint32_t GetUniformBlockIndex(std::string_view name) const noexcept;

	void SetUniform(std::string_view name, int32_t value) const noexcept;
	void SetUniform(std::string_view name, float value) const noexcept;
	void SetUniform(std::string_view name, const glm::vec3& value) const noexcept;
	void SetUniform(std::string_view name, const glm::vec4& value) const noexcept;
	void SetUniform(std::string_view name, const glm::mat4& value) const noexcept;

	[[nodiscard]] explicit operator bool() const noexcept
	{
		return m_program != 0;
	}

private:
	uint32_t m_program = 0;
	mutable std::unordered_map<std::string, int32_t> m_uniformCache;
	mutable std::unordered_map<std::string, uint32_t> m_blockIndexCache;

	void invalidateCache() noexcept
	{
		m_uniformCache.clear();
		m_blockIndexCache.clear();
	}

	[[nodiscard]] static uint32_t compileShader(uint32_t type, std::string_view source);
};

class ShaderException final : public std::runtime_error
{
public:
	using std::runtime_error::runtime_error;
};
