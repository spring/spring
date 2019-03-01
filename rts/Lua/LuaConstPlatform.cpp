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
	LuaPushNamedNumber(L, "gpuMemorySize", globalRenderingInfo.gpuMemorySize.x);
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

	LuaPushNamedBool(L, "glSupportNonPowerOfTwoTex", globalRendering->supportNonPowerOfTwoTex);
	LuaPushNamedBool(L, "glSupportTextureQueryLOD" , globalRendering->supportTextureQueryLOD);
	LuaPushNamedBool(L, "glSupport24bitDepthBuffer", globalRendering->support24bitDepthBuffer);
	LuaPushNamedBool(L, "glSupportRestartPrimitive", globalRendering->supportRestartPrimitive);
	LuaPushNamedBool(L, "glSupportClipSpaceControl", globalRendering->supportClipSpaceControl);
	LuaPushNamedBool(L, "glSupportFragDepthLayout" , globalRendering->supportFragDepthLayout);
	LuaPushNamedBool(L, "glSupportSeamlessCubeMaps", globalRendering->supportSeamlessCubeMaps);

	LuaPushNamedString(L, "osName", Platform::GetOSNameStr());
	LuaPushNamedString(L, "osVersion", Platform::GetOSVersionStr());
	LuaPushNamedString(L, "osFamily", Platform::GetOSFamilyStr());
	LuaPushNamedString(L, "hwConfig", Platform::GetHardwareStr());

	LuaPushNamedString(L, "sysInfoHash", Platform::GetSysInfoHash());
	LuaPushNamedString(L, "macAddrHash", Platform::GetMacAddrHash());

	return true;
}
