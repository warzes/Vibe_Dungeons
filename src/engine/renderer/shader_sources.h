#pragma once

//=============================================================================
// Screen quad shaders (fullscreen FBO blit)
//=============================================================================

#if defined(__EMSCRIPTEN__)

static constexpr const char* SCREEN_VERT_SRC = R"(#version 300 es
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main()
{
	gl_Position = vec4(aPos, 0.0, 1.0);
	vTexCoord = aTexCoord;
}
)";

static constexpr const char* SCREEN_FRAG_SRC = R"(#version 300 es
precision highp float;
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D uTexture;
void main()
{
	fragColor = texture(uTexture, vTexCoord);
}
)";

#else

static constexpr const char* SCREEN_VERT_SRC = R"(#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main()
{
	gl_Position = vec4(aPos, 0.0, 1.0);
	vTexCoord = aTexCoord;
}
)";

static constexpr const char* SCREEN_FRAG_SRC = R"(#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;
uniform sampler2D uTexture;
void main()
{
	fragColor = texture(uTexture, vTexCoord);
}
)";

#endif

//=============================================================================
// Instance cube shaders
//=============================================================================

#if defined(__EMSCRIPTEN__)

static constexpr const char* CUBE_VERT_SRC = R"(#version 300 es
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aModelRow0;
layout (location = 4) in vec4 aModelRow1;
layout (location = 5) in vec4 aModelRow2;
layout (location = 6) in vec4 aModelRow3;

layout(std140) uniform ViewProjection
{
	mat4 uView;
	mat4 uProjection;
};

out vec2 vTexCoord;

void main()
{
	mat4 model = mat4(aModelRow0, aModelRow1, aModelRow2, aModelRow3);
	gl_Position = uProjection * uView * model * vec4(aPos, 1.0);
	vTexCoord = aTexCoord;
}
)";

static constexpr const char* CUBE_FRAG_SRC = R"(#version 300 es
precision highp float;
in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uTexture;

void main()
{
	fragColor = texture(uTexture, vTexCoord);
}
)";

#else

static constexpr const char* CUBE_VERT_SRC = R"(#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec4 aModelRow0;
layout (location = 4) in vec4 aModelRow1;
layout (location = 5) in vec4 aModelRow2;
layout (location = 6) in vec4 aModelRow3;

layout(std140) uniform ViewProjection
{
	mat4 uView;
	mat4 uProjection;
};

out vec2 vTexCoord;

void main()
{
	mat4 model = mat4(aModelRow0, aModelRow1, aModelRow2, aModelRow3);
	gl_Position = uProjection * uView * model * vec4(aPos, 1.0);
	vTexCoord = aTexCoord;
}
)";

static constexpr const char* CUBE_FRAG_SRC = R"(#version 330 core
in vec2 vTexCoord;
out vec4 fragColor;

uniform sampler2D uTexture;

void main()
{
	fragColor = texture(uTexture, vTexCoord);
}
)";

#endif

//=============================================================================
// Debug line shaders
//=============================================================================

#if defined(__EMSCRIPTEN__)

static constexpr const char* DEBUG_VERT_SRC = R"(#version 300 es
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 uViewProj;
out vec3 vColor;
void main()
{
	gl_Position = uViewProj * vec4(aPos, 1.0);
	vColor = aColor;
}
)";

static constexpr const char* DEBUG_FRAG_SRC = R"(#version 300 es
precision highp float;
in vec3 vColor;
out vec4 fragColor;
void main()
{
	fragColor = vec4(vColor, 1.0);
}
)";

#else

static constexpr const char* DEBUG_VERT_SRC = R"(#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
uniform mat4 uViewProj;
out vec3 vColor;
void main()
{
	gl_Position = uViewProj * vec4(aPos, 1.0);
	vColor = aColor;
}
)";

static constexpr const char* DEBUG_FRAG_SRC = R"(#version 330 core
in vec3 vColor;
out vec4 fragColor;
void main()
{
	fragColor = vec4(vColor, 1.0);
}
)";

#endif
