#pragma once

class Window final
{
public:
	Window(
		std::string_view title,
		int32_t width,
		int32_t height
	);
	~Window() noexcept;

	Window(const Window&) = delete;
	Window& operator=(const Window&) = delete;
	Window(Window&&) = delete;
	Window& operator=(Window&&) = delete;

	[[nodiscard]] SDL_Window* Handle() const noexcept
	{
		assert(m_handle != nullptr && "Window handle is null");
		return m_handle;
	}

	[[nodiscard]] int32_t Width() const noexcept
	{
		return m_width;
	}

	[[nodiscard]] int32_t Height() const noexcept
	{
		return m_height;
	}

	void OnResized(int32_t width, int32_t height) noexcept
	{
		m_width = width;
		m_height = height;
	}

	void SwapBuffers() noexcept;

private:
	SDL_Window* m_handle = nullptr;
	int32_t m_width = 0;
	int32_t m_height = 0;
};
