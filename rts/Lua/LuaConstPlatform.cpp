/* This file is part of the Spring System (GPL v2 or later), see LICENSE.html */

#include "LuaConstPlatform.h"
#include "LuaUtils.h"
#include "System/Platform/Misc.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/GlobalRenderingInfo.h"

bool LuaConstPlatform::PushEntries(lua_State* L)
{
	LuaPushNamedString(L, "gpu", globalRenderingInfo.gpuName);
	LuaPushNamedString(L, "gpuVendor", globalRenderingInfo.gpuVendor);
	LuaPushNamedNumber(L, "gpuMemorySize", globalRendering->gpuMemorySize);
	LuaPushNamedString(L, "glVersionShort", globalRenderingInfo.glVersionShort);
	LuaPushNamedString(L, "glslVersionShort", globalRenderingInfo.glslVersionShort);

	LuaPushNamedString(L, "glVersion", globalRenderingInfo.glVersion);
	LuaPushNamedString(L, "glVendor", globalRenderingInfo.glVendor);
	LuaPushNamedString(L, "glRenderer", globalRenderingInfo.glRenderer);
	LuaPushNamedString(L, "glslVersion", globalRenderingInfo.glslVersion);
	LuaPushNamedString(L, "glewVersion", globalRenderingInfo.glewVersion);

	LuaPushNamedNumber(L, "sdlVersionCompiledMajor", globalRenderingInfo.sdlVersionCompiled.major);
	LuaPushNamedNumber(L, "sdlVersionCompiledMinor", globalRenderingInfo.sdlVersionCompiled.minor);
	LuaPushNamedNumber(L, "sdlVersionCompiledPatch", globalRenderingInfo.sdlVersionCompiled.patch);
	LuaPushNamedNumber(L, "sdlVersionLinkedMajor", globalRenderingInfo.sdlVersionLinked.major);
	LuaPushNamedNumber(L, "sdlVersionLinkedMinor", globalRenderingInfo.sdlVersionLinked.minor);
	LuaPushNamedNumber(L, "sdlVersionLinkedPatch", globalRenderingInfo.sdlVersionLinked.patch);

	LuaPushNamedString(L, "osName", Platform::GetOS());
	LuaPushNamedString(L, "osFamily", Platform::GetOSFamily());

	return true;
}
