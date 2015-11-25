/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_OBJECT_DRAWER_H
#define LUA_OBJECT_DRAWER_H

// for LuaObjType and LuaMatType
#include "Lua/LuaObjectMaterial.h"

class CSolidObject;

class LuaMaterial;
class LuaMatBin;

//
// handles drawing of objects (units, features) with
// custom Lua-assigned materials (e.g. display lists,
// shaders, ...) during the various renderer passes
//
// note: custom materials can use standard shaders!
//
class LuaObjectDrawer {
public:
	static bool InDrawPass() { return inDrawPass; }
	static bool DrawSingleObject(CSolidObject* obj, LuaObjType objType);
	static void SetObjectLOD(CSolidObject* obj, LuaObjType objType, unsigned int lodCount);

	static void DrawOpaqueMaterialObjects(LuaObjType objType, LuaMatType matType, bool deferredPass);
	static void DrawAlphaMaterialObjects(LuaObjType objType, LuaMatType matType, bool deferredPass);
	static void DrawShadowMaterialObjects(LuaObjType objType, LuaMatType matType, bool deferredPass);

public:
	static void ReadLODScales(LuaObjType objType);

	static float GetLODScale          () { return (LODScale                     ); }
	static float GetLODScaleShadow    () { return (LODScale * LODScaleShadow    ); }
	static float GetLODScaleReflection() { return (LODScale * LODScaleReflection); }
	static float GetLODScaleRefraction() { return (LODScale * LODScaleRefraction); }

	static float SetLODScale          (float v) { return (LODScale           = v); }
	static float SetLODScaleShadow    (float v) { return (LODScaleShadow     = v); }
	static float SetLODScaleReflection(float v) { return (LODScaleReflection = v); }
	static float SetLODScaleRefraction(float v) { return (LODScaleRefraction = v); }

private:
	static void DrawMaterialBins(LuaObjType objType, LuaMatType matType, bool deferredPass);
	static const LuaMaterial* DrawMaterialBin(
		const LuaMatBin* currBin,
		const LuaMaterial* currMat,
		LuaObjType objType,
		LuaMatType matType,
		bool deferredPass
	);

private:
	static bool inDrawPass;

	static float LODScale;
	static float LODScaleShadow;
	static float LODScaleReflection;
	static float LODScaleRefraction;
};

#endif

