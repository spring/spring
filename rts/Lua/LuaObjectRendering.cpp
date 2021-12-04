/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "LuaObjectRendering.h"
#include "LuaMaterial.h"

#include "LuaInclude.h"
#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"

#include "Rendering/LuaObjectDrawer.h"
#include "Rendering/Models/3DModel.h"
// see ParseUnitTexture
// #include "Rendering/Textures/3DOTextureHandler.h"
// #include "Rendering/Textures/S3OTextureHandler.h"
#include "Sim/Objects/SolidObjectDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/ProjectileHandler.h"

#include "System/Log/ILog.h"
#include "System/StringUtil.h"

static int material_index(lua_State* L)
{
	LuaMatRef** matRef = (LuaMatRef**) luaL_checkudata(L, 1, "MatRef");

	const string key = luaL_checkstring(L, 2);
	const LuaMatBin* bin = (*matRef)->GetBin();

	if (bin == nullptr)
		return 0;

	//const LuaMaterial& mat = *bin;
	//if (key == "type") {
	//}
	return 0;
}

static int material_newindex(lua_State* L)
{
	luaL_checkudata(L, 1, "MatRef");
	return 0;
}

static int material_gc(lua_State* L)
{
	LuaMatRef** matRef = (LuaMatRef**) luaL_checkudata(L, 1, "MatRef");

	delete *matRef;
	return 0;
}




/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

static inline CSolidObject* ParseSolidObject(lua_State* L, const char* caller, int index, int objType)
{
	if (!lua_isnumber(L, index)) {
		luaL_error(L, "[%s] objectID (arg #%d) not a number\n", caller, index);
		return nullptr;
	}

	switch (objType) {
		case LUAOBJ_UNIT   : { return (   unitHandler.GetUnit   (lua_toint(L, index))); } break;
		case LUAOBJ_FEATURE: { return (featureHandler.GetFeature(lua_toint(L, index))); } break;
		default            : {                                           assert(false); } break;
	}

	return nullptr;
}


/*
static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	return (static_cast<CUnit*>(ParseSolidObject(L, caller, index, LUAOBJ_UNIT)));
}

static inline CFeature* ParseFeature(lua_State* L, const char* caller, int index)
{
	return (static_cast<CFeature*>(ParseSolidObject(L, caller, index, LUAOBJ_FEATURE)));
}
*/




/******************************************************************************/
/******************************************************************************/

std::vector<LuaObjType> LuaObjectRenderingImpl::objectTypeStack;



void LuaObjectRenderingImpl::CreateMatRefMetatable(lua_State* L)
{
	luaL_newmetatable(L, "MatRef");
	HSTR_PUSH_CFUNC(L, "__gc",       material_gc);
	HSTR_PUSH_CFUNC(L, "__index",    material_index);
	HSTR_PUSH_CFUNC(L, "__newindex", material_newindex);
	lua_pop(L, 1);
}

void LuaObjectRenderingImpl::PushFunction(lua_State* L, int (*fnPntr)(lua_State*), const char* fnName)
{
	lua_pushstring(L, fnName);
	lua_pushcfunction(L, fnPntr);
	lua_rawset(L, -3);
}



int LuaObjectRenderingImpl::GetLODCount(lua_State* L)
{
	const CSolidObject* obj = ParseSolidObject(L, __func__, 1, GetObjectType());

	if (obj == nullptr)
		return 0;

	const LuaObjectMaterialData* lmd = obj->GetLuaMaterialData();

	lua_pushnumber(L, lmd->GetLODCount());
	lua_pushnumber(L, lmd->GetCurrentLOD());
	return 2;
}

int LuaObjectRenderingImpl::SetLODCount(lua_State* L)
{
	// args=<objID, lodCount>
	const unsigned int objType = GetObjectType();
	const unsigned int lodCount = std::min(1024, luaL_checkint(L, 2));

	CSolidObject* obj = ParseSolidObject(L, __func__, 1, objType);

	if (obj == nullptr)
		return 0;

	LuaObjectDrawer::SetObjectLOD(obj, LuaObjType(objType), lodCount);
	return 0;
}


static int SetLODLengthCommon(lua_State* L, CSolidObject* obj, float scale)
{
	if (obj == nullptr)
		return 0;

	LuaObjectMaterialData* lmd = obj->GetLuaMaterialData();

	// actual Length-Per-Pixel
	lmd->SetLODLength(luaL_checknumber(L, 2) - 1, luaL_checkfloat(L, 3) * scale);
	return 0;
}

int LuaObjectRenderingImpl::SetLODLength(lua_State* L)
{
	// args=<objID, lodLevel, lodLength>
	return (SetLODLengthCommon(L, ParseSolidObject(L, __func__, 1, GetObjectType()), 1.0f));
}

int LuaObjectRenderingImpl::SetLODDistance(lua_State* L)
{
	// args=<objID, lodLevel, lodLength>
	//
	// length adjusted for 45 degree FOV with a 1024x768 screen; the magic
	// constant is 2.0f * math::tanf((45.0f * 0.5f) * (PI / 180.0f)) / 768.0f)
	return (SetLODLengthCommon(L, ParseSolidObject(L, __func__, 1, GetObjectType()), 0.0010786811520132682f));
}


/******************************************************************************/
/******************************************************************************/

static LuaMatType ParseMaterialType(const char* matName)
{
	switch (hashString(matName)) {
		case hashString("alpha"         ): { return LUAMAT_ALPHA         ; } break;
		case hashString("opaque"        ): { return LUAMAT_OPAQUE        ; } break;
		case hashString("alpha_reflect" ): { return LUAMAT_ALPHA_REFLECT ; } break;
		case hashString("opaque_reflect"): { return LUAMAT_OPAQUE_REFLECT; } break;
		case hashString("shadow"        ): { return LUAMAT_SHADOW        ; } break;
		default                          : {                               } break;
	}

	LOG_L(L_WARNING, "[%s] unknown Lua material type \"%s\"", __func__, matName);
	return (LuaMatType) -1; 
}


static LuaObjectMaterial* GetObjectMaterial(CSolidObject* obj, const char* matName)
{
	LuaMatType matType = ParseMaterialType(matName);
	LuaObjectMaterialData* lmd = obj->GetLuaMaterialData();

	if (matType < 0)
		return nullptr;

	return (lmd->GetLuaMaterial(matType));
}


/******************************************************************************/

static void ParseShader(lua_State* L, int index, LuaMatShader& shader)
{
	const LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);

	switch (lua_type(L, index)) {
		case LUA_TNUMBER: {
			shader.SetCustomTypeFromID(shaders.GetProgramName(luaL_checknumber(L, index)));
		} break;

		case LUA_TSTRING: {
			shader.SetEngineTypeFromKey(lua_tostring(L, index));
		} break;

		default: {
		} break;
	}
}

static void ParseTexture(lua_State* L, int index, LuaMatTexture& texUnit) {
	if (index < 0)
		index = lua_gettop(L) + index + 1;

	if (lua_isstring(L, index)) {
		LuaOpenGLUtils::ParseTextureImage(L, texUnit, lua_tostring(L, index));
		texUnit.Enable(true);
		return;
	}

	if (!lua_istable(L, index))
		return;

	const int table = (index > 0) ? index : (lua_gettop(L) + index + 1);

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2))
			continue;

		const string key = StringToLower(lua_tostring(L, -2));

		if (key == "tex") {
			LuaOpenGLUtils::ParseTextureImage(L, texUnit, lua_tostring(L, -1));
			texUnit.Enable(true);
			continue;
		}

		if (key == "enable") {
			texUnit.Enable(lua_isboolean(L, -1) && lua_toboolean(L, -1));
			continue;
		}
	}
}

static GLuint ParseDisplayList(lua_State* L, int index)
{
	return 0;
}

static LuaMatRef ParseMaterial(lua_State* L, int index, LuaMatType matType) {
	if (!lua_istable(L, index))
		return LuaMatRef();

	LuaMaterial mat(matType);
	mat.Parse(L, index, &ParseShader, &ParseTexture, &ParseDisplayList);
	mat.Finalize();
	return (luaMatHandler.GetRef(mat));
}


/******************************************************************************/
/******************************************************************************/

int LuaObjectRenderingImpl::GetMaterial(lua_State* L)
{
	const LuaMatType matType = ParseMaterialType(luaL_checkstring(L, 1));

	if (!lua_istable(L, 2))
		luaL_error(L, "Incorrect arguments to GetMaterial");

	LuaMatRef** matRef = (LuaMatRef**) lua_newuserdata(L, sizeof(LuaMatRef*));
	luaL_getmetatable(L, "MatRef");
	lua_setmetatable(L, -2);

	*matRef = new LuaMatRef;
	**matRef = ParseMaterial(L, 2, matType);

	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaObjectRenderingImpl::SetMaterial(lua_State* L)
{
	// args=<objID, lodMatNum, matName, matRef>
	CSolidObject* obj = ParseSolidObject(L, __func__, 1, GetObjectType());

	if (obj == nullptr)
		return 0;

	const char* matName = luaL_checkstring(L, 3);
	const LuaMatType matType = ParseMaterialType(matName);

	LuaObjectMaterial* objMat = GetObjectMaterial(obj, matName);

	if (objMat == nullptr)
		return 0;

	LuaMatRef* lodMatRef = objMat->GetMaterial(luaL_checkint(L, 2) - 1);

	if (lodMatRef == nullptr)
		return 0;

	if (lua_isuserdata(L, 4)) {
		LuaMatRef** matRef = (LuaMatRef**) luaL_checkudata(L, 4, "MatRef");

		if (matRef != nullptr)
			*lodMatRef = **matRef;

	} else {
		*lodMatRef = ParseMaterial(L, 4, matType);
	}

	return 0;
}


int LuaObjectRenderingImpl::SetMaterialLastLOD(lua_State* L)
{
	// args=<objID, matName, lodMatNum>
	CSolidObject* obj = ParseSolidObject(L, __func__, 1, GetObjectType());

	if (obj == nullptr)
		return 0;

	LuaObjectMaterial* objMat = GetObjectMaterial(obj, luaL_checkstring(L, 2));

	if (objMat == nullptr)
		return 0;

	objMat->SetLastLOD(luaL_checknumber(L, 3) - 1);
	return 0;
}


static int SetMaterialUniform(lua_State* L, LuaObjType objType, LuaMatShader::Pass matPass)
{
	// args=<objID, matName, lodMatNum,   uniformName, uniformType, uniformData>
	CSolidObject* obj = ParseSolidObject(L, __func__, 1, objType);

	LuaObjectMaterial* objMat = nullptr;
	LuaMatRef* lodMatRef = nullptr;
	LuaMatBin* matBin = nullptr;

	const char* materialName = luaL_checkstring(L, 2);
	const char*  uniformName = luaL_checkstring(L, 4);

	if (obj == nullptr)
		return 0;

	if ((objMat = GetObjectMaterial(obj, materialName)) == nullptr)
		return 0;
	if ((lodMatRef = objMat->GetMaterial(luaL_checkint(L, 3) - 1)) == nullptr)
		return 0;
	if ((matBin = lodMatRef->GetBin()) == nullptr)
		return 0;

	LuaMatUniforms& matUniforms = matBin->uniforms[matPass];
	// find an existing uniform to update
	LuaMatUniform& objUniform = matUniforms.GetObjectUniform(lua_toint(L, 1), objType, uniformName);
	LuaMatUniform& dmyUniform = LuaMatUniforms::GetDummyObjectUniform();

	constexpr size_t S = sizeof(objUniform.name);
	constexpr size_t N = sizeof(objUniform.data.i) / sizeof(objUniform.data.i[0]);

	if (&objUniform == &dmyUniform) {
		memset(objUniform.data.i, 0, sizeof(objUniform.data.i));
		memset(objUniform.name, 0, S);
		strncpy(objUniform.name, uniformName, S - 1);
		assert(objUniform.loc == -3);

		// if u is a dummy reference, it counts as a new uniform
		// actual location will be set when the material executes
		objUniform.loc = -2;
	}

	switch (objUniform.type = luaL_checkint(L, 5)) {
		case GL_INT       : { objUniform.size = LuaUtils::ParseIntArray  (L, 6, objUniform.data.i, N) / 1; } break;
		case GL_INT_VEC2  : { objUniform.size = LuaUtils::ParseIntArray  (L, 6, objUniform.data.i, N) / 2; } break;
		case GL_INT_VEC3  : { objUniform.size = LuaUtils::ParseIntArray  (L, 6, objUniform.data.i, N) / 3; } break;
		case GL_INT_VEC4  : { objUniform.size = LuaUtils::ParseIntArray  (L, 6, objUniform.data.i, N) / 4; } break;

		case GL_FLOAT     : { objUniform.size = LuaUtils::ParseFloatArray(L, 6, objUniform.data.f, N) / 1; } break;
		case GL_FLOAT_VEC2: { objUniform.size = LuaUtils::ParseFloatArray(L, 6, objUniform.data.f, N) / 2; } break;
		case GL_FLOAT_VEC3: { objUniform.size = LuaUtils::ParseFloatArray(L, 6, objUniform.data.f, N) / 3; } break;
		case GL_FLOAT_VEC4: { objUniform.size = LuaUtils::ParseFloatArray(L, 6, objUniform.data.f, N) / 4; } break;

		case GL_FLOAT_MAT3: { objUniform.size = LuaUtils::ParseFloatArray(L, 6, objUniform.data.f, N) /  9; } break;
		case GL_FLOAT_MAT4: { objUniform.size = LuaUtils::ParseFloatArray(L, 6, objUniform.data.f, N) / 16; } break;

		default: {
			return 0;
		} break;
	}

	lua_pushboolean(L, &objUniform != &dmyUniform || matUniforms.AddObjectUniform(lua_toint(L, 1), objType, objUniform));
	return 1;
}

int LuaObjectRenderingImpl::SetDeferredMaterialUniform(lua_State* L) { return (SetMaterialUniform(L, GetObjectType(), LuaMatShader::LUASHADER_PASS_DFR)); }
int LuaObjectRenderingImpl::SetForwardMaterialUniform(lua_State* L) { return (SetMaterialUniform(L, GetObjectType(), LuaMatShader::LUASHADER_PASS_FWD)); }


static int ClearMaterialUniform(lua_State* L, LuaObjType objType, LuaMatShader::Pass matPass)
{
	// args=<objID, matName, lodMatNum,   uniformName>
	CSolidObject* obj = ParseSolidObject(L, __func__, 1, objType);

	LuaObjectMaterial* objMat = nullptr;
	LuaMatRef* lodMatRef = nullptr;
	LuaMatBin* matBin = nullptr;
	LuaMatUniforms* matUniforms = nullptr;

	const char* materialName = luaL_checkstring(L, 2);
	const char*  uniformName = luaL_checkstring(L, 4);

	if (obj == nullptr)
		return 0;

	if ((objMat = GetObjectMaterial(obj, materialName)) == nullptr)
		return 0;
	if ((lodMatRef = objMat->GetMaterial(luaL_checkint(L, 3) - 1)) == nullptr)
		return 0;
	if ((matBin = lodMatRef->GetBin()) == nullptr)
		return 0;
	if ((matUniforms = &matBin->uniforms[matPass]) == nullptr)
		return 0;

	switch (hashString(uniformName)) {
		case hashString("$obj"): { lua_pushboolean(L, matUniforms->ClearObjectUniforms(obj->id, objType             )); } break; // clears all uniforms for one object
		case hashString("$all"): { lua_pushboolean(L, matUniforms->ClearObjectUniforms(         objType             )); } break; // clears all uniforms for all objects
		default                : { lua_pushboolean(L, matUniforms->ClearObjectUniform (obj->id, objType, uniformName)); } break; // clears one uniform for one object by name
	}

	return 1;
}

int LuaObjectRenderingImpl::ClearDeferredMaterialUniform(lua_State* L) { return (ClearMaterialUniform(L, GetObjectType(), LuaMatShader::LUASHADER_PASS_FWD)); }
int LuaObjectRenderingImpl::ClearForwardMaterialUniform(lua_State* L) { return (ClearMaterialUniform(L, GetObjectType(), LuaMatShader::LUASHADER_PASS_DFR)); }


/******************************************************************************/
/******************************************************************************/

template<typename ObjectType>
static int SetObjectLuaDraw(lua_State* L, ObjectType* obj)
{
	if (obj == nullptr)
		return 0;

	if (!lua_isboolean(L, 2))
		return 0;

	obj->luaDraw = lua_toboolean(L, 2);
	return 0;
}


int LuaObjectRenderingImpl::SetUnitLuaDraw(lua_State* L)
{
	return (SetObjectLuaDraw<CSolidObject>(L, unitHandler.GetUnit(luaL_checkint(L, 1))));
}

int LuaObjectRenderingImpl::SetFeatureLuaDraw(lua_State* L)
{
	return (SetObjectLuaDraw<CSolidObject>(L, featureHandler.GetFeature(luaL_checkint(L, 1))));
}

int LuaObjectRenderingImpl::SetProjectileLuaDraw(lua_State* L)
{
	return (SetObjectLuaDraw<CProjectile>(L, projectileHandler.GetProjectileBySyncedID(luaL_checkint(L, 1))));
}

/******************************************************************************/
/******************************************************************************/

static void PrintObjectLOD(const CSolidObject* obj, int lod)
{
	const LuaObjectMaterialData* lmd = obj->GetLuaMaterialData();
	const LuaObjectMaterial* mats = lmd->GetLuaMaterials();

	LOG("  LOD %i:", lod);
	LOG("    LodLength = %f", lmd->GetLODLength(lod));

	for (int type = 0; type < LUAMAT_TYPE_COUNT; type++) {
		const LuaObjectMaterial& luaMat = mats[type];
		const LuaMatRef* lodMatRef = luaMat.GetMaterial(lod);
		const LuaMatBin* bin = lodMatRef->GetBin();

		if (bin == nullptr)
			continue;

		bin->Print("    ");
	}
}


int LuaObjectRenderingImpl::Debug(lua_State* L)
{
	if (lua_gettop(L) == 0) {
		// no arguments, dump all bins
		luaMatHandler.PrintAllBins("");
		return 0;
	}

	// args=<objID>
	const CSolidObject* obj = ParseSolidObject(L, __func__, 1, GetObjectType());

	if (obj == nullptr)
		return 0;

	const LuaObjectMaterialData* lmd = obj->GetLuaMaterialData();
	const LuaObjectMaterial* mats = lmd->GetLuaMaterials();

	LOG_L(L_DEBUG, "%s", "");
	LOG_L(L_DEBUG, "ObjectID      = %i", obj->id);
	LOG_L(L_DEBUG, "ObjectDefID   = %i", obj->GetDef()->id);
	LOG_L(L_DEBUG, "ObjectDefName = %s", obj->GetDef()->name.c_str());
	LOG_L(L_DEBUG, "LodCount      = %i", lmd->GetLODCount());
	LOG_L(L_DEBUG, "CurrentLod    = %i", lmd->GetCurrentLOD());
	LOG_L(L_DEBUG, "%s", "");

	const LuaObjectMaterial& alphaMat      = mats[LUAMAT_ALPHA];
	const LuaObjectMaterial& opaqueMat     = mats[LUAMAT_OPAQUE];
	const LuaObjectMaterial& alphaReflMat  = mats[LUAMAT_ALPHA_REFLECT];
	const LuaObjectMaterial& opaqueReflMat = mats[LUAMAT_OPAQUE_REFLECT];
	const LuaObjectMaterial& shadowMat     = mats[LUAMAT_SHADOW];

	LOG_L(L_DEBUG, "LUAMAT_ALPHA          lastLOD = %i", alphaMat.GetLastLOD());
	LOG_L(L_DEBUG, "LUAMAT_OPAQUE         lastLOD = %i", opaqueMat.GetLastLOD());
	LOG_L(L_DEBUG, "LUAMAT_ALPHA_REFLECT  lastLOD = %i", alphaReflMat.GetLastLOD());
	LOG_L(L_DEBUG, "LUAMAT_OPAQUE_REFLECT lastLOD = %i", opaqueReflMat.GetLastLOD());
	LOG_L(L_DEBUG, "LUAMAT_SHADOW         lastLOD = %i", shadowMat.GetLastLOD());

	for (unsigned lod = 0; lod < lmd->GetLODCount(); lod++) {
		PrintObjectLOD(obj, lod);
	}

	return 0;
}

/******************************************************************************/
/******************************************************************************/

