#include "stdafx.h"
#include "engine/renderer/shader.h"

Shader::~Shader() noexcept
{
	if (m_program != 0)
	{
		glDeleteProgram(m_program);
	}
}

void Shader::LoadFromSource(std::string_view vertexSource, std::string_view fragmentSource)
{
	if (m_program != 0)
	{
		glDeleteProgram(m_program);
		m_program = 0;
	}

	const auto vs = compileShader(GL_VERTEX_SHADER, vertexSource);
	const auto fs = compileShader(GL_FRAGMENT_SHADER, fragmentSource);

	m_program = glCreateProgram();
	if (m_program == 0)
	{
		glDeleteShader(vs);
		glDeleteShader(fs);
		throw ShaderException("glCreateProgram failed");
	}
	glAttachShader(m_program, vs);
	glAttachShader(m_program, fs);
	glLinkProgram(m_program);

	GLint success = 0;
	glGetProgramiv(m_program, GL_LINK_STATUS, &success);
	if (!success)
	{
		GLint logLength = 0;
		glGetProgramiv(m_program, GL_INFO_LOG_LENGTH, &logLength);
		std::string log(logLength, '\0');
		glGetProgramInfoLog(m_program, logLength, nullptr, log.data());
		glDeleteProgram(m_program);
		m_program = 0;
		throw ShaderException("Shader link error: " + log);
	}

	glDeleteShader(vs);
	glDeleteShader(fs);

	invalidateCache();
}

void Shader::Bind() const noexcept
{
	glUseProgram(m_program);
}

void Shader::Unbind() noexcept
{
	glUseProgram(0);
}

int32_t Shader::GetUniformLocation(std::string_view name) const noexcept
{
	std::string key(name);
	auto it = m_uniformCache.find(key);
	if (it != m_uniformCache.end())
	{
		return it->second;
	}
	const int32_t location = glGetUniformLocation(m_program, key.c_str());
	m_uniformCache[key] = location;
	return location;
}

void Shader::SetUniform(std::string_view name, int32_t value) const noexcept
{
	glUniform1i(GetUniformLocation(name), value);
}

void Shader::SetUniform(std::string_view name, float value) const noexcept
{
	glUniform1f(GetUniformLocation(name), value);
}

void Shader::SetUniform(std::string_view name, const glm::vec3& value) const noexcept
{
	glUniform3fv(GetUniformLocation(name), 1, &value.x);
}

void Shader::SetUniform(std::string_view name, const glm::vec4& value) const noexcept
{
	glUniform4fv(GetUniformLocation(name), 1, &value.x);
}

void Shader::SetUniform(std::string_view name, const glm::mat4& value) const noexcept
{
	glUniformMatrix4fv(GetUniformLocation(name), 1, GL_FALSE, glm::value_ptr(value));
}

uint32_t Shader::compileShader(uint32_t type, std::string_view source)
{
	uint32_t shader = glCreateShader(type);
	assert(shader != 0 && "glCreateShader returned 0");
	const char* src = source.data();
	GLint len = static_cast<GLint>(std::min(source.size(), static_cast<size_t>(INT_MAX)));
	glShaderSource(shader, 1, &src, &len);
	glCompileShader(shader);

	GLint success = 0;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		GLint logLength = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		std::string log(logLength, '\0');
		glGetShaderInfoLog(shader, logLength, nullptr, log.data());
		glDeleteShader(shader);

		const char* typeName = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
		throw ShaderException(std::string(typeName) + " shader compile error: " + log);
	}
	return shader;
}
