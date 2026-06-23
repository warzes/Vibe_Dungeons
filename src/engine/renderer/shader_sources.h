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

//=============================================================================
// Dungeon shaders (ambient + directional light)
//=============================================================================

#if defined(__EMSCRIPTEN__)

static constexpr const char* DUNGEON_VERT_SRC = R"(#version 300 es
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
out vec3 vNormal;
out vec3 vFragPos;

void main()
{
	mat4 model = mat4(aModelRow0, aModelRow1, aModelRow2, aModelRow3);
	vec4 worldPos = model * vec4(aPos, 1.0);
	gl_Position = uProjection * uView * worldPos;
	vTexCoord = aTexCoord;
	vNormal = normalize(mat3(transpose(inverse(model))) * aNormal);
	vFragPos = vec3(worldPos);
}
)";

static constexpr const char* DUNGEON_FRAG_SRC = R"(#version 300 es
precision highp float;
in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vFragPos;
out vec4 fragColor;

uniform sampler2D uTexture;

void main()
{
	vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
	float diff = max(dot(vNormal, lightDir), 0.0);
	float ambient = 0.3;
	float lighting = ambient + (1.0 - ambient) * diff;
	vec4 texColor = texture(uTexture, vTexCoord);
	if (texColor.a < 0.01) discard;
	fragColor = vec4(texColor.rgb * lighting, texColor.a);
}
)";

#else

static constexpr const char* DUNGEON_VERT_SRC = R"(#version 330 core
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
out vec3 vNormal;
out vec3 vFragPos;

void main()
{
	mat4 model = mat4(aModelRow0, aModelRow1, aModelRow2, aModelRow3);
	vec4 worldPos = model * vec4(aPos, 1.0);
	gl_Position = uProjection * uView * worldPos;
	vTexCoord = aTexCoord;
	vNormal = normalize(mat3(transpose(inverse(model))) * aNormal);
	vFragPos = vec3(worldPos);
}
)";

static constexpr const char* DUNGEON_FRAG_SRC = R"(#version 330 core
in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vFragPos;
out vec4 fragColor;

uniform sampler2D uTexture;

void main()
{
	vec3 lightDir = normalize(vec3(0.5, 1.0, 0.3));
	float diff = max(dot(vNormal, lightDir), 0.0);
	float ambient = 0.3;
	float lighting = ambient + (1.0 - ambient) * diff;
	vec4 texColor = texture(uTexture, vTexCoord);
	if (texColor.a < 0.01) discard;
	fragColor = vec4(texColor.rgb * lighting, texColor.a);
}
)";

#endif

//=============================================================================
// Overworld shaders (daylight, brighter ambient)
//=============================================================================

#if defined(__EMSCRIPTEN__)

static constexpr const char* OVERWORLD_VERT_SRC = R"(#version 300 es
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
out vec3 vNormal;
out vec3 vFragPos;

void main()
{
	mat4 model = mat4(aModelRow0, aModelRow1, aModelRow2, aModelRow3);
	vec4 worldPos = model * vec4(aPos, 1.0);
	gl_Position = uProjection * uView * worldPos;
	vTexCoord = aTexCoord;
	vNormal = normalize(mat3(transpose(inverse(model))) * aNormal);
	vFragPos = vec3(worldPos);
}
)";

static constexpr const char* OVERWORLD_FRAG_SRC = R"(#version 300 es
precision highp float;
in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vFragPos;
out vec4 fragColor;

uniform sampler2D uTexture;
uniform float uAmbientLight;

void main()
{
	vec3 lightDir = normalize(vec3(0.0, 1.0, 0.3));
	float diff = max(dot(vNormal, lightDir), 0.0);
	float ambient = uAmbientLight;
	float lighting = ambient + (1.0 - ambient) * diff;
	vec4 texColor = texture(uTexture, vTexCoord);
	if (texColor.a < 0.01) discard;
	fragColor = vec4(texColor.rgb * lighting, texColor.a);
}
)";

#else

static constexpr const char* OVERWORLD_VERT_SRC = R"(#version 330 core
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
out vec3 vNormal;
out vec3 vFragPos;

void main()
{
	mat4 model = mat4(aModelRow0, aModelRow1, aModelRow2, aModelRow3);
	vec4 worldPos = model * vec4(aPos, 1.0);
	gl_Position = uProjection * uView * worldPos;
	vTexCoord = aTexCoord;
	vNormal = normalize(mat3(transpose(inverse(model))) * aNormal);
	vFragPos = vec3(worldPos);
}
)";

static constexpr const char* OVERWORLD_FRAG_SRC = R"(#version 330 core
in vec2 vTexCoord;
in vec3 vNormal;
in vec3 vFragPos;
out vec4 fragColor;

uniform sampler2D uTexture;
uniform float uAmbientLight;

void main()
{
	vec3 lightDir = normalize(vec3(0.0, 1.0, 0.3));
	float diff = max(dot(vNormal, lightDir), 0.0);
	float ambient = uAmbientLight;
	float lighting = ambient + (1.0 - ambient) * diff;
	vec4 texColor = texture(uTexture, vTexCoord);
	if (texColor.a < 0.01) discard;
	fragColor = vec4(texColor.rgb * lighting, texColor.a);
}
)";

#endif

//=============================================================================
// Animation billboard shaders (colored quad, no texture)
//=============================================================================

#if defined(__EMSCRIPTEN__)

static constexpr const char* ANIM_VERT_SRC = R"(#version 300 es
layout (location = 0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uViewProj;

void main()
{
	gl_Position = uViewProj * uModel * vec4(aPos, 1.0);
}
)";

static constexpr const char* ANIM_FRAG_SRC = R"(#version 300 es
precision highp float;
out vec4 fragColor;

uniform vec4 uColor;

void main()
{
	fragColor = uColor;
}
)";

#else

static constexpr const char* ANIM_VERT_SRC = R"(#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 uModel;
uniform mat4 uViewProj;

void main()
{
	gl_Position = uViewProj * uModel * vec4(aPos, 1.0);
}
)";

static constexpr const char* ANIM_FRAG_SRC = R"(#version 330 core
out vec4 fragColor;

uniform vec4 uColor;

void main()
{
	fragColor = uColor;
}
)";

#endif
