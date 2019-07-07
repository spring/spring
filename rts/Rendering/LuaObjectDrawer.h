/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_OBJECT_DRAWER_H
#define LUA_OBJECT_DRAWER_H

// for LuaObjType and LuaMatType
#include "Lua/LuaObjectMaterial.h"
#include "Rendering/GL/GeometryBuffer.h"

class CSolidObject;

class LuaMaterial;
class LuaMatBin;
class LuaMatShader;

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
	static void DrawDeferredPass(LuaObjType objType);

	static bool DrawSingleObjectCommon(const CSolidObject* obj, LuaObjType objType, bool applyTrans);
	static bool DrawSingleObject(const CSolidObject* obj, LuaObjType objType);
	static bool DrawSingleObjectLuaTrans(const CSolidObject* obj, LuaObjType objType);

	static void Update(bool init);
	static void Init();
	static void Kill();

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
	static void SetDrawPassGlobalLODFactor(LuaObjType objType);

	static int GetBinObjTeam() { return binObjTeam; }

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

	// shared by {Unit,Feature}Drawer
	// note: the pointer MUST already be valid during
	// FeatureDrawer's ctor, in CWorldDrawer::LoadPre)
	static GL::GeometryBuffer* GetGeometryBuffer() { return geomBuffer; }

private:
	static void DrawMaterialBins(LuaObjType objType, LuaMatType matType, bool deferredPass);
	static void DrawMaterialBin(
		const LuaMatBin* currBin,
		const LuaMaterial* prevMat,
		LuaObjType objType,
		LuaMatType matType,
		bool deferredPass,
		bool alphaMatBin
	);

	static void DrawBinObject(
		const CSolidObject* obj,
		LuaObjType objType,
		const LuaMatRef* matRef,
		const LuaMaterial* luaMat,
		bool deferredPass,
		bool alphaMatBin,
		bool applyTrans,
		bool noLuaCall
	);

private:
	static GL::GeometryBuffer* geomBuffer;

	// whether we are currently in DrawMaterialBins
	static bool inDrawPass;
	// whether alpha-material bins are being drawn
	static bool inAlphaBin;

	// whether we can execute DrawDeferredPass
	static bool drawDeferredEnabled;
	// whether deferred object drawing is allowed by user
	static bool drawDeferredAllowed;

	// whether the deferred feature pass clears the GB
	static bool bufferClearAllowed;

	// team of last object visited in DrawMaterialBin
	// (needed because bins are not sorted by team and
	// Lua shaders do not have any uniform caching yet)
	static int binObjTeam;

	static float LODScale[LUAOBJ_LAST];
	static float LODScaleShadow[LUAOBJ_LAST];
	static float LODScaleReflection[LUAOBJ_LAST];
	static float LODScaleRefraction[LUAOBJ_LAST];
};

#endif

