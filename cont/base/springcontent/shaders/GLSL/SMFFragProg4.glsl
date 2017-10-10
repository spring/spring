#version 410 core

#ifdef NOSPRING
	#define SMF_INTENSITY_MULT (210.0 / 255.0)
	#define SMF_TEXSQUARE_SIZE 1024.0
	#define MAX_DYNAMIC_MAP_LIGHTS 4
	#define BASE_DYNAMIC_MAP_LIGHT 0
	#define GBUFFER_NORMTEX_IDX 0
	#define GBUFFER_DIFFTEX_IDX 1
	#define GBUFFER_SPECTEX_IDX 2
	#define GBUFFER_EMITTEX_IDX 3
	#define GBUFFER_MISCTEX_IDX 4
#endif


in vec4 vertexWorldPos;

#ifdef DEFERRED_MODE
layout(location = 0) out vec4 fragData[6];
#else
out vec4 fragColor;
#endif


void main() {
	#ifdef DEFERRED_MODE
	vec3 scaledNormal = (vec3(0.0, 1.0, 0.0) + vec3(1.0, 1.0, 1.0)) * 0.5;

	fragData[GBUFFER_NORMTEX_IDX] = vec4(scaledNormal, 1.0);
	fragData[GBUFFER_DIFFTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);
	fragData[GBUFFER_SPECTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);
	fragData[GBUFFER_EMITTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);
	fragData[GBUFFER_MISCTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);
	#else
	fragColor = vec4(1.0, 0.0, 0.0, 1.0);
	#endif
}

