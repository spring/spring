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
	static bool AddObjectForLOD(CSolidObject* obj, LuaObjType objType, bool useAlphaMat, bool useShadowMat);

	static bool AddOpaqueMaterialObject(CSolidObject* obj, LuaObjType objType);
	static bool AddAlphaMaterialObject(CSolidObject* obj, LuaObjType objType);
	static bool AddShadowMaterialObject(CSolidObject* obj, LuaObjType objType);

	static void DrawOpaqueMaterialObjects(LuaObjType objType, bool deferredPass);
	static void DrawAlphaMaterialObjects(LuaObjType objType, bool deferredPass);
	static void DrawShadowMaterialObjects(LuaObjType objType, bool deferredPass);

public:
	static void ReadLODScales(LuaObjType objType);
	static void SetGlobalDrawPassLODFactor(LuaObjType objType);

	static float GetLODScale          (int objType) { return (LODScale[objType]                              ); }
	static float GetLODScaleShadow    (int objType) { return (LODScale[objType] * LODScaleShadow    [objType]); }
	static float GetLODScaleReflection(int objType) { return (LODScale[objType] * LODScaleReflection[objType]); }
	static float GetLODScaleRefraction(int objType) { return (LODScale[objType] * LODScaleRefraction[objType]); }

	static float SetLODScale          (int objType, float v) { return (LODScale          [objType] = v); }
	static float SetLODScaleShadow    (int objType, float v) { return (LODScaleShadow    [objType] = v); }
	static float SetLODScaleReflection(int objType, float v) { return (LODScaleReflection[objType] = v); }
	static float SetLODScaleRefraction(int objType, float v) { return (LODScaleRefraction[objType] = v); }

	static LuaMatType GetDrawPassOpaqueMat();
	static LuaMatType GetDrawPassAlphaMat();
	static LuaMatType GetDrawPassShadowMat() { return LUAMAT_SHADOW; }

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

	static float LODScale[LUAOBJ_LAST];
	static float LODScaleShadow[LUAOBJ_LAST];
	static float LODScaleReflection[LUAOBJ_LAST];
	static float LODScaleRefraction[LUAOBJ_LAST];
};

#endif

