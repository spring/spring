/* This file is part of the Spring System (GPL v2 or later), see LICENSE.html */

#include "LuaConstSystem.h"
#include "LuaUtils.h"
#include "System/Platform/Misc.h"
#include "Rendering/GlobalRendering.h"

#include <SDL.h>

bool LuaConstSystem::PushEntries(lua_State* L)
{
	LuaPushNamedString(L, "gpu", globalRendering->gpu);
	LuaPushNamedString(L, "gpuVendor", globalRendering->gpuVendor);
	LuaPushNamedNumber(L, "gpuMemorySize", globalRendering->gpuMemorySize);
	LuaPushNamedString(L, "glVersionShort", globalRendering->glVersionShort);
	LuaPushNamedString(L, "glslVersionShort", globalRendering->glslVersionShort);

	LuaPushNamedString(L, "glVersion", globalRendering->glVersion);
	LuaPushNamedString(L, "glVendor", globalRendering->glVendor);
	LuaPushNamedString(L, "glRenderer", globalRendering->glRenderer);
	LuaPushNamedString(L, "glslVersion", globalRendering->glslVersion);
	LuaPushNamedString(L, "glewVersion", globalRendering->glewVersion);

	LuaPushNamedNumber(L, "sdlVersionCompiledMajor", globalRendering->sdlVersionCompiled->major);
	LuaPushNamedNumber(L, "sdlVersionCompiledMinor", globalRendering->sdlVersionCompiled->minor);
	LuaPushNamedNumber(L, "sdlVersionCompiledPatch", globalRendering->sdlVersionCompiled->patch);
	LuaPushNamedNumber(L, "sdlVersionLinkedMajor", globalRendering->sdlVersionLinked->major);
	LuaPushNamedNumber(L, "sdlVersionLinkedMinor", globalRendering->sdlVersionLinked->minor);
	LuaPushNamedNumber(L, "sdlVersionLinkedPatch", globalRendering->sdlVersionLinked->patch);

	LuaPushNamedString(L, "os", Platform::GetOS());
	LuaPushNamedString(L, "osFamily", Platform::GetOSFamily());
	LuaPushNamedNumber(L, "osWordSize", Platform::NativeWordSize()*8);

	return true;
}
