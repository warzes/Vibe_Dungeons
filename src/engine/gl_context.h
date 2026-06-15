#pragma once

class GLContext final
{
public:
	explicit GLContext(SDL_Window* window);
	~GLContext() noexcept;

	GLContext(const GLContext&) = delete;
	GLContext& operator=(const GLContext&) = delete;
	GLContext(GLContext&& other) noexcept;
	GLContext& operator=(GLContext&& other) noexcept;

	[[nodiscard]] SDL_GLContext Handle() const noexcept
	{
		assert(m_context != nullptr && "GLContext handle is null");
		return m_context;
	}

	void SetVSync(bool enabled) noexcept;

private:
	SDL_GLContext m_context = nullptr;
};
