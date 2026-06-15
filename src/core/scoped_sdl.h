#pragma once

#include "core/exception.h"
#include "core/logger.h"

class ScopedSDL final
{
public:
	explicit ScopedSDL(uint32_t flags = SDL_INIT_VIDEO)
	{
		if (!SDL_Init(flags))
		{
			THROW_ENGINE(std::string("SDL_Init failed: ") + SDL_GetError());
		}
		Logger::Info("SDL initialized");
	}

	~ScopedSDL() noexcept
	{
		SDL_Quit();
		Logger::Info("SDL quit");
	}

	ScopedSDL(const ScopedSDL&) = delete;
	ScopedSDL& operator=(const ScopedSDL&) = delete;
	ScopedSDL(ScopedSDL&&) = delete;
	ScopedSDL& operator=(ScopedSDL&&) = delete;
};
