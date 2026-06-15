#include "stdafx.h"
#include "engine/window.h"
#include "core/exception.h"

Window::Window(std::string_view title, int32_t width, int32_t height)
	: m_width(width)
	, m_height(height)
{
	const std::string titleStr(title);
	m_handle = SDL_CreateWindow(
		titleStr.c_str(),
		width,
		height,
		SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE
	);

	if (!m_handle)
	{
		THROW_ENGINE(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
	}
}

Window::~Window() noexcept
{
	if (m_handle)
	{
		SDL_DestroyWindow(m_handle);
	}
}

void Window::SwapBuffers() noexcept
{
	SDL_GL_SwapWindow(m_handle);
}
