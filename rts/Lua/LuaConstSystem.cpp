/* This file is part of the Spring System (GPL v2 or later), see LICENSE.html */

#include "LuaConstSystem.h"
#include "LuaUtils.h"
#include "System/Platform/Misc.h"
#include "Rendering/GlobalRendering.h"

bool LuaConstSystem::PushEntries(lua_State* L)
{
	LuaPushNamedString(L, "gpu", globalRendering->gpu);
	LuaPushNamedString(L, "gpuVendor", globalRendering->gpuVendor);
	LuaPushNamedNumber(L, "gpuMemorySize", globalRendering->gpuMemorySize);
	LuaPushNamedString(L, "glslShaderLevel", globalRendering->glslShaderLevel);
	LuaPushNamedString(L, "glVersion", globalRendering->glVersion);

	LuaPushNamedString(L, "glVersionFull", globalRendering->glVersionFull);
	LuaPushNamedString(L, "glVendor", globalRendering->glVendor);
	LuaPushNamedString(L, "glRenderer", globalRendering->glRenderer);
	LuaPushNamedString(L, "glslVersion", globalRendering->glslVersion);
	LuaPushNamedString(L, "glewVersion", globalRendering->glewVersion);

	LuaPushNamedString(L, "os", Platform::GetOS());
	LuaPushNamedString(L, "osFamily", Platform::GetOSFamily());
	LuaPushNamedNumber(L, "osWordSize", Platform::NativeWordSize()*8);

	return true;
}
