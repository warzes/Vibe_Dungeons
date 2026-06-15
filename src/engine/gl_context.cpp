#include "stdafx.h"
#include "engine/gl_context.h"
#include "core/exception.h"
#include "core/logger.h"

GLContext::GLContext(SDL_Window* window)
{
#if defined(__EMSCRIPTEN__)
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
#else
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

	m_context = SDL_GL_CreateContext(window);
	if (!m_context)
	{
		THROW_ENGINE(std::string("SDL_GL_CreateContext failed: ") + SDL_GetError());
	}

#ifndef __EMSCRIPTEN__
	if (!gladLoadGL(reinterpret_cast<GLADloadfunc>(SDL_GL_GetProcAddress)))
	{
		SDL_GL_DestroyContext(m_context);
		m_context = nullptr;
		THROW_ENGINE("Failed to load OpenGL functions via glad");
	}
#endif

	glEnable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	SetVSync(true);

	Logger::Info("OpenGL initialized:");
	Logger::Info(std::string("  Vendor: ") + reinterpret_cast<const char*>(glGetString(GL_VENDOR)));
	Logger::Info(std::string("  Renderer: ") + reinterpret_cast<const char*>(glGetString(GL_RENDERER)));
	Logger::Info(std::string("  Version: ") + reinterpret_cast<const char*>(glGetString(GL_VERSION)));
}

GLContext::~GLContext() noexcept
{
	if (m_context)
	{
		SDL_GL_DestroyContext(m_context);
	}
}

GLContext::GLContext(GLContext&& other) noexcept
	: m_context(other.m_context)
{
	other.m_context = nullptr;
}

GLContext& GLContext::operator=(GLContext&& other) noexcept
{
	if (this != &other)
	{
		if (m_context)
		{
			SDL_GL_DestroyContext(m_context);
		}
		m_context = other.m_context;
		other.m_context = nullptr;
	}
	return *this;
}

void GLContext::SetVSync(bool enabled) noexcept
{
	SDL_GL_SetSwapInterval(enabled ? 1 : 0);
}
