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
#include "Sim/Units/UnitHandler.h"
#include "Sim/Features/FeatureHandler.h"
#include "System/Log/ILog.h"
#include "System/Util.h"



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
		case LUAOBJ_UNIT   : { return (   unitHandler->GetUnit   (lua_toint(L, index))); } break;
		case LUAOBJ_FEATURE: { return (featureHandler->GetFeature(lua_toint(L, index))); } break;
		default            : {                                            assert(false); } break;
	}

	return nullptr;
}


static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	return (static_cast<CUnit*>(ParseSolidObject(L, caller, index, LUAOBJ_UNIT)));
}

static inline CFeature* ParseFeature(lua_State* L, const char* caller, int index)
{
	return (static_cast<CFeature*>(ParseSolidObject(L, caller, index, LUAOBJ_FEATURE)));
}




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
	const CSolidObject* obj = ParseSolidObject(L, __FUNCTION__, 1, GetObjectType());

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

	CSolidObject* obj = ParseSolidObject(L, __FUNCTION__, 1, objType);

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
	return (SetLODLengthCommon(L, ParseSolidObject(L, __FUNCTION__, 1, GetObjectType()), 1.0f));
}

int LuaObjectRenderingImpl::SetLODDistance(lua_State* L)
{
	// args=<objID, lodLevel, lodLength>
	//
	// length adjusted for 45 degree FOV with a 1024x768 screen; the magic
	// constant is 2.0f * math::tanf((45.0f * 0.5f) * (PI / 180.0f)) / 768.0f)
	return (SetLODLengthCommon(L, ParseSolidObject(L, __FUNCTION__, 1, GetObjectType()), 0.0010786811520132682f));
}


/******************************************************************************/

int LuaObjectRenderingImpl::SetPieceList(lua_State* L)
{
	CSolidObject* obj = ParseSolidObject(L, __FUNCTION__, 1, GetObjectType());

	if (obj == nullptr)
		return 0;

	const LuaObjectMaterialData* lmd = obj->GetLuaMaterialData();
	LocalModelPiece* lmp = ParseObjectLocalModelPiece(L, obj, 3);

	if (lmp == nullptr)
		return 0;

	const unsigned int lod = luaL_checknumber(L, 2) - 1;

	if (lod >= lmd->GetLODCount())
		return 0;

	// (re)set the default if no fourth argument
	unsigned int dlist = lmp->dispListID;

	if (lua_isnumber(L, 4)) {
		CLuaDisplayLists& displayLists = CLuaHandle::GetActiveDisplayLists(L);
		dlist = displayLists.GetDList(luaL_checknumber(L, 4));
	}

	lmp->lodDispLists[lod] = dlist;
	return 0;
}


/******************************************************************************/
/******************************************************************************/

static const map<string, LuaMatType>& GetMatNameMap()
{
	static map<string, LuaMatType> matNameMap;
	if (matNameMap.empty()) {
		matNameMap["alpha"]          = LUAMAT_ALPHA;
		matNameMap["opaque"]         = LUAMAT_OPAQUE;
		matNameMap["alpha_reflect"]  = LUAMAT_ALPHA_REFLECT;
		matNameMap["opaque_reflect"] = LUAMAT_OPAQUE_REFLECT;
		matNameMap["shadow"]         = LUAMAT_SHADOW;
	}
	return matNameMap;
}


static LuaMatType ParseMaterialType(const string& matName)
{
	const string lower = StringToLower(matName);
	const auto it = GetMatNameMap().find(lower);

	if (it == GetMatNameMap().end())
		return (LuaMatType) -1;

	return it->second;
}


static LuaObjectMaterial* GetObjectMaterial(CSolidObject* obj, const string& matName)
{
	LuaMatType matType = ParseMaterialType(matName);
	LuaObjectMaterialData* lmd = obj->GetLuaMaterialData();

	if (matType < 0)
		return nullptr;

	return (lmd->GetLuaMaterial(matType));
}


/******************************************************************************/

static void ParseShader(lua_State* L, const char* caller, int index, LuaMatShader& shader)
{
	const LuaShaders& shaders = CLuaHandle::GetActiveShaders(L);

	switch (lua_type(L, index)) {
		case LUA_TNUMBER: {
			shader.SetTypeFromID(shaders.GetProgramName(luaL_checknumber(L, index)));
		} break;

		case LUA_TSTRING: {
			shader.SetTypeFromKey(StringToLower(lua_tostring(L, index)));
		} break;

		default: {
		} break;
	}
}

/*
static GLuint ParseUnitTexture(const string& texture)
{
	if (texture.length() < 4)
		return 0;

	char* endPtr;
	const char* startPtr = texture.c_str() + 1; // skip the '%'
	const int id = (int)strtol(startPtr, &endPtr, 10);
	if ((endPtr == startPtr) || (*endPtr != ':')) {
		return 0;
	}

	endPtr++; // skip the ':'
	if ( (startPtr-1)+texture.length() <= endPtr ) {
		return 0; // ':' is end of string, but we expect '%num:0'
	}

	if (id == 0) {
		if (*endPtr == '0') {
			return texturehandler3DO->GetAtlasTex1ID();
		}
		else if (*endPtr == '1') {
			return texturehandler3DO->GetAtlasTex2ID();
		}
		return 0;
	}

	S3DModel* model;

	if (id < 0) {
		const FeatureDef* fd = featureHandler->GetFeatureDefByID(-id);
		if (fd == nullptr) {
			return 0;
		}
		model = fd->LoadModel();
	} else {
		const UnitDef* ud = unitDefHandler->GetUnitDefByID(id);
		if (ud == nullptr) {
			return 0;
		}
		model = ud->LoadModel();
	}

	if (model == nullptr) {
		return 0;
	}

	const unsigned int texType = model->textureType;
	if (texType == 0) {
		return 0;
	}

	const CS3OTextureHandler::S3OTexMat* stex = texturehandlerS3O->GetTexture(texType);
	if (stex == nullptr) {
		return 0;
	}

	if (*endPtr == '0') {
		return stex->tex1;
	}
	else if (*endPtr == '1') {
		return stex->tex2;
	}

	return 0;
}
*/

static void ParseTexture(lua_State* L, const char* caller, int index,
                        LuaMatTexture& texUnit)
{
	if (index < 0)
		index = lua_gettop(L) + index + 1;

	if (lua_isstring(L, index)) {
		LuaOpenGLUtils::ParseTextureImage(L, texUnit, lua_tostring(L, index));
		texUnit.enable = true;
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
			texUnit.enable = true;
		}
		else if (key == "enable") {
			if (lua_isboolean(L, -1)) {
				texUnit.enable = lua_toboolean(L, -1);
			}
		}
	}
}


static GLuint ParseDisplayList(lua_State* L, const char* caller, int index)
{
	if (lua_isnumber(L, index)) {
		const unsigned int ilist = (unsigned int)luaL_checknumber(L, index);
		const CLuaDisplayLists& displayLists = CLuaHandle::GetActiveDisplayLists(L);
		return displayLists.GetDList(ilist);
	}
	return 0;
}


static LuaMatRef ParseMaterial(lua_State* L, const char* caller, int index,
                               LuaMatType matType)
{
	if (!lua_istable(L, index))
		return LuaMatRef();

	LuaMaterial mat;
	mat.type = matType;

	const int table = index;

	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2)) {
			continue;
		}
		const string key = StringToLower(lua_tostring(L, -2));

		if (key == "order") {
			mat.order = luaL_checkint(L, -1);
		}

		else if (key == "standard_shader" || key == "shader") {
			ParseShader(L, caller, -1, mat.shaders[LuaMatShader::LUASHADER_PASS_FWD]);
		}
		else if (key == "deferred_shader" || key == "deferred") {
			ParseShader(L, caller, -1, mat.shaders[LuaMatShader::LUASHADER_PASS_DFR]);
		}

		else if (key == "texunits") {
			if (lua_istable(L, -1)) {
				const int texTable = (int)lua_gettop(L);
				for (lua_pushnil(L); lua_next(L, texTable) != 0; lua_pop(L, 1)) {
					if (lua_israwnumber(L, -2)) {
						const int texUnit = lua_toint(L, -2);
						if ((texUnit >= 0) && (texUnit < LuaMatTexture::maxTexUnits)) {
							ParseTexture(L, caller, -1, mat.textures[texUnit]);
						}
					}
				}
			}
		}
		else if (key.substr(0, 7) == "texunit") {
			const int texUnit = atoi(key.c_str() + 7);
			if ((texUnit >= 0) && (texUnit < LuaMatTexture::maxTexUnits)) {
				ParseTexture(L, caller, -1, mat.textures[texUnit]);
			}
		}
		else if (key == "prelist") {
			mat.preList = ParseDisplayList(L, caller, -1);
		}
		else if (key == "postlist") {
			mat.postList = ParseDisplayList(L, caller, -1);
		}
		else if (key == "usecamera") {
			if (lua_isboolean(L, -1)) {
				mat.useCamera = lua_toboolean(L, -1);
			}
		}
		else if (key == "culling") {
			if (lua_isnumber(L, -1)) {
				mat.culling = (GLenum)lua_tonumber(L, -1);
			}
		}

		else if (key == "cameraloc") {
			if (lua_isnumber(L, -1)) {
				mat.cameraLoc = (GLint)lua_tonumber(L, -1);
			}
		}
		else if (key == "camerainvloc") {
			if (lua_isnumber(L, -1)) {
				mat.cameraInvLoc = (GLint)lua_tonumber(L, -1);
			}
		}
		else if (key == "cameraposloc") {
			if (lua_isnumber(L, -1)) {
				mat.cameraPosLoc = (GLint)lua_tonumber(L, -1);
			}
		}

		else if (key == "sunposloc") {
			if (lua_isnumber(L, -1)) {
				mat.sunPosLoc = (GLint)lua_tonumber(L, -1);
			}
		}
		else if (key == "shadowloc") {
			if (lua_isnumber(L, -1)) {
				mat.shadowLoc = (GLint)lua_tonumber(L, -1);
			}
		}
		else if (key == "shadowparamsloc") {
			if (lua_isnumber(L, -1)) {
				mat.shadowParamsLoc = (GLint)lua_tonumber(L, -1);
			}
		}
	}

	mat.Finalize();
	return luaMatHandler.GetRef(mat);
}


/******************************************************************************/
/******************************************************************************/

int LuaObjectRenderingImpl::GetMaterial(lua_State* L)
{
	const LuaMatType matType = ParseMaterialType(luaL_checkstring(L, 1));

	if (!lua_istable(L, 2)) {
		luaL_error(L, "Incorrect arguments to GetMaterial");
	}

	LuaMatRef** matRef = (LuaMatRef**) lua_newuserdata(L, sizeof(LuaMatRef*));
	luaL_getmetatable(L, "MatRef");
	lua_setmetatable(L, -2);

	*matRef = new LuaMatRef;
	**matRef = ParseMaterial(L, __FUNCTION__, 2, matType);

	return 1;
}


/******************************************************************************/
/******************************************************************************/

int LuaObjectRenderingImpl::SetMaterial(lua_State* L)
{
	// args=<objID, lodMatNum, matName, matRef>
	CSolidObject* obj = ParseSolidObject(L, __FUNCTION__, 1, GetObjectType());

	if (obj == nullptr)
		return 0;

	const string matName = luaL_checkstring(L, 3);
	const LuaMatType matType = ParseMaterialType(matName);

	LuaObjectMaterial* objMat = GetObjectMaterial(obj, matName);

	if (objMat == nullptr)
		return 0;

	LuaObjectLODMaterial* lodMat = objMat->GetMaterial(luaL_checknumber(L, 2) - 1);

	if (lodMat == nullptr)
		return 0;

	if (lua_isuserdata(L, 4)) {
		LuaMatRef** matRef = (LuaMatRef**) luaL_checkudata(L, 4, "MatRef");
		if (matRef) {
			lodMat->matref = **matRef;
		}
	} else {
		lodMat->matref = ParseMaterial(L, __FUNCTION__, 4, matType);
	}

	return 0;
}


int LuaObjectRenderingImpl::SetMaterialLastLOD(lua_State* L)
{
	// args=<objID, matName, lodMatNum>
	CSolidObject* obj = ParseSolidObject(L, __FUNCTION__, 1, GetObjectType());

	if (obj == nullptr)
		return 0;

	LuaObjectMaterial* objMat = GetObjectMaterial(obj, luaL_checkstring(L, 2));

	if (objMat == nullptr)
		return 0;

	objMat->SetLastLOD(luaL_checknumber(L, 3) - 1);
	return 0;
}


int LuaObjectRenderingImpl::SetMaterialDisplayLists(lua_State* L)
{
	// args=<objID, lodLevel, matName, preListID, postListID>
	CSolidObject* obj = ParseSolidObject(L, __FUNCTION__, 1, GetObjectType());

	if (obj == nullptr)
		return 0;

	LuaObjectMaterial* objMat = GetObjectMaterial(obj, luaL_checkstring(L, 3));

	if (objMat == nullptr)
		return 0;

	LuaObjectLODMaterial* lodMat = objMat->GetMaterial(luaL_checknumber(L, 2) - 1);

	if (lodMat == nullptr)
		return 0;

	lodMat->preDisplayList  = ParseDisplayList(L, __FUNCTION__, 4);
	lodMat->postDisplayList = ParseDisplayList(L, __FUNCTION__, 5);
	return 0;
}


int LuaObjectRenderingImpl::SetObjectUniform(lua_State* L)
{
	/*
	// args=<objID, matName, lodLevel [, ...]>
	CSolidObject* obj = ParseSolidObject(L, __FUNCTION__, 1, GetObjectType());

	if (obj == nullptr)
		return 0;

	LuaObjectMaterial* objMat = GetObjectMaterial(obj, luaL_checkstring(L, 2));

	if (objMat == nullptr)
		return 0;

	LuaObjectLODMaterial* lodMat = objMat->GetMaterial(luaL_checknumber(L, 3) - 1);

	if (lodMat == nullptr)
		return 0;

	const int args = lua_gettop(L) - 3;
	const int lastArg = std::min(args, lodMat->uniforms.customCount + 3);

	for (int i = 3; i <= lastArg; i++) {
		// FIXME: Set all lods at once?
	}
	*/

	return 0;
}

/******************************************************************************/
/******************************************************************************/

static int SetObjectLuaDraw(lua_State* L, CSolidObject* obj)
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
	return (SetObjectLuaDraw(L, unitHandler->GetUnit(luaL_checkint(L, 1))));
}

int LuaObjectRenderingImpl::SetFeatureLuaDraw(lua_State* L)
{
	return (SetObjectLuaDraw(L, featureHandler->GetFeature(luaL_checkint(L, 1))));
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
		const LuaObjectLODMaterial* lodMat = luaMat.GetMaterial(lod);
		const LuaMatBin* bin = lodMat->matref.GetBin();

		if (bin) {
			bin->Print("    ");
		}
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
	const CSolidObject* obj = ParseSolidObject(L, __FUNCTION__, 1, GetObjectType());

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

