/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_RENDERING_INFO_H
#define _GLOBAL_RENDERING_INFO_H

#include <SDL_version.h>
#include "System/type2.h"

#include <vector>

struct GlobalRenderingInfo {
	struct AvailableVideoMode {
		std::string displayName;
		int32_t displayIndex;
		int32_t width;
		int32_t height;
		int32_t bpp;
		int32_t refreshRate;
	};

	const char* gpuName;
	const char* gpuVendor;

	const char* glVersion;
	const char* glVendor;
	const char* glRenderer;
	const char* glslVersion;
	const char* glewVersion;

	char glVersionShort[256];
	char glslVersionShort[256];

	bool glContextIsCore;
	int2 glContextVersion;
	int2 gpuMemorySize;

	SDL_version sdlVersionCompiled;
	SDL_version sdlVersionLinked;

	std::vector<AvailableVideoMode> availableVideoModes;
};

extern GlobalRenderingInfo globalRenderingInfo;

#endif

