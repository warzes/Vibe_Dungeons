#pragma once

#if defined(_MSC_VER)
#	pragma warning(disable : 4514)
#	pragma warning(disable : 4623)
#	pragma warning(disable : 4625)
#	pragma warning(disable : 4626)
#	pragma warning(disable : 4820)
#	pragma warning(disable : 5026)
#	pragma warning(disable : 5027)
#	pragma warning(disable : 5045)
#	pragma warning(push, 3)
#	pragma warning(disable : 5262)
#endif

#define _USE_MATH_DEFINES

#if defined(__EMSCRIPTEN__)
#	include <emscripten.h>
#endif

// glad MUST be included before any system GL headers
#if defined(__EMSCRIPTEN__)
#	include <GLES3/gl3.h>
#	include <GLES3/gl2ext.h>
#else
#	include <glad/gl.h>
#endif

#if defined(__EMSCRIPTEN__)
// No CWD-changing needed on Web — Emscripten virtual FS handles paths.
#elif defined(_WIN32)
#	include <direct.h>
#else
#	include <unistd.h>
#endif

#include <cassert>
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <cstring>

#include <algorithm>
#include <array>
#include <chrono>
#include <exception>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <ranges>
#include <sstream>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>
#include <atomic>
#include <source_location>

#include <SDL3/SDL.h>
#include <SDL3/SDL_keyboard.h>
#include <SDL3/SDL_mouse.h>
#include <SDL3/SDL_audio.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

#include <imgui/imgui.h>
#include <imgui/imgui_impl_sdl3.h>
#include <imgui/imgui_impl_opengl3.h>

#include <nlohmann/json.hpp>

#include <tiny_obj_loader/tiny_obj_loader.h>
#include <cgltf/cgltf.h>

#include <stb/stb_image.h>

#if defined(_MSC_VER)
#	pragma warning(pop)
#endif