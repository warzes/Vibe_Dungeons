#include "stdafx.h"
#include "game/game_app.h"
#include "core/scoped_sdl.h"
#include "core/json_data_manager.h"
//=============================================================================
#if !defined(__EMSCRIPTEN__)
#	define RUN_TESTS
#	if defined(RUN_TESTS)
#		include "test/test_all.h"
#	endif
#endif
//=============================================================================
#if defined(_MSC_VER)
#	pragma comment( lib, "SDL3.lib" )
#endif
//https://github.com/lonski/amarlon
//=============================================================================
int main(int argc, char* argv[])
{
	(void)argc;

#if !defined(__EMSCRIPTEN__)
	// Change CWD to executable directory so relative paths (config, assets)
	// resolve consistently regardless of how the game is launched.
	if (argv[0] && argv[0][0] != '\0')
	{
		std::string exePath = argv[0];
		size_t sep = exePath.find_last_of("\\/");
		if (sep != std::string::npos)
		{
			std::string dir = exePath.substr(0, sep);
			_chdir(dir.c_str());
		}
	}
#endif

#if defined(RUN_TESTS)
	int result = TestRunner::RunAll("test_results.log");
	std::printf("[TEST] Log written to test_results.log\n");
	if (result != 0)
	{
		std::printf("[TEST] Some tests FAILED — exiting.\n");
		std::getchar();
		return 1;
	}
	std::printf("[TEST] All tests passed — continuing to game.\n");
#endif

	// Load JSON data files
	if (!JsonDataManager::Instance().LoadAll("data"))
	{
#if defined(__EMSCRIPTEN__)
		EM_ASM({ alert("[ERROR] Failed to load JSON data files. Check --preload-file."); });
#else
		std::printf("[ERROR] Failed to load one or more JSON data files.\n");
		std::getchar();
#endif
		return 1;
	}
	std::printf("[DATA] All JSON data files loaded successfully.\n");

	try
	{
		ScopedSDL sdl;
		(void)sdl;
		GameApp app;
		app.Run();
		return 0;
	}
	catch (const EngineException& e)
	{
		Logger::Fatal(
			std::string("Engine exception: ") + e.what() +
			" at " + e.Location().file_name() +
			":" + std::to_string(e.Location().line())
		);
	}
	catch (const std::exception& e)
	{
		Logger::Fatal(std::string("Unhandled exception: ") + e.what());
	}
	catch (...)
	{
		Logger::Fatal("Unknown fatal error");
	}
	return 1;
}
//=============================================================================