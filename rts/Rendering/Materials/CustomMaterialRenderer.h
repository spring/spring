#pragma once

#include "CustomMaterial.h"
#include "Lua/LuaObjectMaterial.h"

template<LuaObjType lot>
class CustomMaterialRenderer {
public:
	static void DrawOpaqueMaterialObjects(bool deferredPass, bool drawReflection, bool drawRefraction) {};
	static void DrawAlphaMaterialObjects(bool drawReflection, bool drawRefraction) {};
};