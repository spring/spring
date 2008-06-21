#include "StdAfx.h"
// LuaUnitRendering.cpp: implementation of the CLuaUnitRendering class.
//
//////////////////////////////////////////////////////////////////////

#include <cmath>

#include "LuaUnitRendering.h"
#include "LuaMaterial.h"

#include "LuaInclude.h"

#include "LuaHandle.h"
#include "LuaHashString.h"
#include "LuaUtils.h"

#include "Rendering/Textures/TextureHandler.h"
#include "Rendering/Textures/NamedTextures.h"
#include "Rendering/UnitModels/3DModelParser.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Rendering/UnitModels/s3oParser.h"
#include "Rendering/UnitModels/UnitDrawer.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "LogOutput.h"

namespace std {using ::tanf;} // for some reason MSVS needs this

using std::min;
using std::max;

static void CreateMatRefMetatable(lua_State* L);


/******************************************************************************/
/******************************************************************************/

bool LuaUnitRendering::PushEntries(lua_State* L)
{
	CreateMatRefMetatable(L);

#define REGISTER_LUA_CFUNC(x) \
	lua_pushstring(L, #x);      \
	lua_pushcfunction(L, x);    \
	lua_rawset(L, -3)

	REGISTER_LUA_CFUNC(SetLODCount);
	REGISTER_LUA_CFUNC(SetLODLength);
	REGISTER_LUA_CFUNC(SetLODDistance);

	REGISTER_LUA_CFUNC(SetPieceList);

	REGISTER_LUA_CFUNC(GetMaterial);

	REGISTER_LUA_CFUNC(SetMaterial);
	REGISTER_LUA_CFUNC(SetMaterialLastLOD);
	REGISTER_LUA_CFUNC(SetMaterialDisplayLists);

	REGISTER_LUA_CFUNC(SetUnitUniform);

	REGISTER_LUA_CFUNC(SetUnitLuaDraw);

	REGISTER_LUA_CFUNC(Debug);

	return true;
}


/******************************************************************************/
/******************************************************************************/
//
//  Parsing helpers
//

static inline CUnit* ParseUnit(lua_State* L, const char* caller, int index)
{
	if (!lua_isnumber(L, index)) {
		if (caller != NULL) {
			luaL_error(L, "Bad unitID parameter in %s()\n", caller);
		} else {
			return NULL;
		}
	}
	const int unitID = (int)lua_tonumber(L, index);
	if ((unitID < 0) || (unitID >= MAX_UNITS)) {
		luaL_error(L, "%s(): Bad unitID: %i\n", caller, unitID);
	}
	CUnit* unit = uh->units[unitID];
	if (unit == NULL) {
		return NULL;
	}
	return unit;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnitRendering::SetLODCount(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const unsigned int count = (unsigned int)luaL_checknumber(L, 2);
	if (count > 1024) {
		luaL_error(L, "SetLODCount() ridiculous lod count");
	}
	unit->SetLODCount(count);
	return 0;
}


int LuaUnitRendering::SetLODLength(lua_State* L)
{
	// Actual Length-Per-Pixel
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const unsigned int lod = (unsigned int)luaL_checknumber(L, 2) - 1;
	if (lod >= unit->lodCount) {
		return 0;
	}
	const float lpp = (float)luaL_checknumber(L, 3);
	unit->lodLengths[lod] = lpp;
	return 0;
}

int LuaUnitRendering::SetLODDistance(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const unsigned int lod = (unsigned int)luaL_checknumber(L, 2) - 1;
	if (lod >= unit->lodCount) {
		return 0;
	}
	// adjusted for 45 degree FOV with a 1024x768 screen
	const float scale = 2.0f * (float)std::tanf((45.0 * 0.5) * (PI / 180.0)) / 768.0f;
	const float dist = (float)luaL_checknumber(L, 3);
	unit->lodLengths[lod] = dist * scale;
	return 0;
}


/******************************************************************************/

int LuaUnitRendering::SetPieceList(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if ((unit == NULL) || (unit->localmodel == NULL)) {
		return 0;
	}
	const LocalS3DOModel* localModel = unit->localmodel;

	const unsigned int lod   = (unsigned int)luaL_checknumber(L, 2) - 1;
	if (lod >= unit->lodCount) {
		return 0;
	}
	const unsigned int piece = (unsigned int)luaL_checknumber(L, 3) - 1;
	if (piece >= localModel->numpieces) {
		return 0;
	}
	LocalS3DO& localPiece = localModel->pieces[piece];

	unsigned int dlist = 0;
	if (lua_isnumber(L, 4)) {
		const unsigned int ilist = (unsigned int)luaL_checknumber(L, 4);
		CLuaDisplayLists& displayLists = CLuaHandle::GetActiveDisplayLists();
		dlist = displayLists.GetDList(ilist);
	} else {
		dlist = localPiece.displist; // set to the default
	}

	localPiece.lodDispLists[lod] = dlist;

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
	map<string, LuaMatType>::const_iterator it;
	it = GetMatNameMap().find(lower);
	if (it == GetMatNameMap().end()) {
		return (LuaMatType) -1;
	}
	return it->second;
}


static LuaUnitMaterial* GetUnitMaterial(CUnit* unit, const string& matName)
{
	LuaMatType type = ParseMaterialType(matName);
	if (type < 0) {
		return NULL;
	}
	return &unit->luaMats[type];
}


/******************************************************************************/

static void ParseShader(lua_State* L, const char* caller, int index,
                          LuaMatShader& shader)
{
	const int luaType = lua_type(L, index);
	if (luaType == LUA_TNUMBER) {
		const unsigned int ishdr = (unsigned int)luaL_checknumber(L, index);
		const LuaShaders& shaders = CLuaHandle::GetActiveShaders();
		const unsigned int id = shaders.GetProgramName(ishdr);
		if (id == 0) {
			shader.openglID = 0;
			shader.type = LuaMatShader::LUASHADER_NONE;
		} else {
			shader.openglID = id;
			shader.type = LuaMatShader::LUASHADER_GL;
		}
	}
	else if (luaType == LUA_TSTRING) {
		const string key = StringToLower(lua_tostring(L, index));
		if (key == "3do") {
			shader.type = LuaMatShader::LUASHADER_3DO;
		}
		else if (key == "s3o") {
			shader.type = LuaMatShader::LUASHADER_S3O;
		}
	}
	return;
}


static GLuint ParseUnitTexture(const string& texture)
{
	char* endPtr;
	const char* startPtr = texture.c_str() + 1; // skip the '%'
	const int unitDefID = (int)strtol(startPtr, &endPtr, 10);
	if ((endPtr == startPtr) || (*endPtr != ':')) {
		return 0;
	}
	const UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
	if (ud == NULL) {
		return 0;
	}
	S3DOModel* model = ud->LoadModel(0);
	const unsigned int texType = model->textureType;
	if (texType == 0) {
		return 0;
	}

	const CTextureHandler::S3oTex* stex = texturehandler->GetS3oTex(texType);
	if (stex == NULL) {
		return 0;
	}

	endPtr++; // skip the ':'
	if (*endPtr == '0') {
		return stex->tex1;
	}
	else if (*endPtr == '1') {
		return stex->tex2;
	}

	return 0;
}


static void ParseTextureImage(LuaMatTexture& texUnit, const string& image)
{
	GLuint texID = 0;

	if (image[0] == '%') {
		texID = ParseUnitTexture(image);
	}
	else if (image[0] == '#') {
		// unit build picture
		char* endPtr;
		const char* startPtr = image.c_str() + 1; // skip the '#'
		const int unitDefID = (int)strtol(startPtr, &endPtr, 10);
		if (endPtr == startPtr) {
			return;
		}
		const UnitDef* ud = unitDefHandler->GetUnitByID(unitDefID);
		if (ud == NULL) {
			return;
		}
		texID = unitDefHandler->GetUnitImage(ud);
	}
	else if (image[0] == LuaTextures::prefix) {
		// dynamic texture
		LuaTextures& textures = CLuaHandle::GetActiveTextures();
		const LuaTextures::Texture* texInfo = textures.GetInfo(image);
		if (texInfo != NULL) {
			texID = texInfo->id;
		}
	}
	else if (image[0] == '$') {
		if (image == "$units") {
			texUnit.type = LuaMatTexture::LUATEX_GL;
			texUnit.openglID = texturehandler->GetGlobalTexID();
		}
		else if (image == "$shadow") {
			texUnit.type = LuaMatTexture::LUATEX_SHADOWMAP;
		}
		else if (image == "$specular") {
			texUnit.type = LuaMatTexture::LUATEX_SPECULAR;
		}
		else if (image == "$reflection") {
			texUnit.type = LuaMatTexture::LUATEX_REFLECTION;
		}
		return;
	}
	else {
		const CNamedTextures::TexInfo* texInfo = CNamedTextures::GetInfo(image);
		if (texInfo != NULL) {
			texID = texInfo->id;
		}
	}

	if (texID != 0) {
		texUnit.type = LuaMatTexture::LUATEX_GL;
		texUnit.openglID = texID;
	}

	return;
}


static void ParseTexture(lua_State* L, const char* caller, int index,
                        LuaMatTexture& texUnit)
{
	if (index < 0) {
		index = lua_gettop(L) + index + 1;
	}

	if (lua_isstring(L, index)) {
		const string texName = lua_tostring(L, index);
		ParseTextureImage(texUnit, texName);
		texUnit.enable = true;
		return;
	}

	if (!lua_istable(L, index)) {
		return;
	}

	const int table = (index > 0) ? index : (lua_gettop(L) + index + 1);
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2)) {
			continue;
		}
		const string key = StringToLower(lua_tostring(L, -2));
		if (key == "tex") {
			const string texName = lua_tostring(L, -1);
			ParseTextureImage(texUnit, texName);
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
		const CLuaDisplayLists& displayLists = CLuaHandle::GetActiveDisplayLists();
		return displayLists.GetDList(ilist);
	}
	return 0;
}


static LuaMatRef ParseMaterial(lua_State* L, const char* caller, int index,
                               LuaMatType matType)
{
	if (!lua_istable(L, index)) {
		return LuaMatRef();
	}
	LuaMaterial mat;
	mat.type = matType;
	const int table = index;
	for (lua_pushnil(L); lua_next(L, table) != 0; lua_pop(L, 1)) {
		if (!lua_israwstring(L, -2)) {
			continue;
		}
		const string key = StringToLower(lua_tostring(L, -2));
		if (key == "order") {
			mat.order = (int)luaL_checknumber(L, -1);
		}
		else if (key == "shader") {
			ParseShader(L, caller, -1, mat.shader);
		}
		else if (key == "texunits") {
			if (lua_istable(L, -1)) {
			  const int texTable = (int)lua_gettop(L);
				for (lua_pushnil(L); lua_next(L, texTable) != 0; lua_pop(L, 1)) {
					if (lua_israwnumber(L, -2)) {
						const int texUnit = (int)lua_tonumber(L, -2);
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
		else if (key == "cameraloc") {
			if (lua_isnumber(L, -1)) {
				mat.cameraLoc = (GLint)lua_tonumber(L, -1);
			}
		}
	}

	mat.Finalize();
	return luaMatHandler.GetRef(mat);
}


/******************************************************************************/
/******************************************************************************/

int LuaUnitRendering::GetMaterial(lua_State* L)
{
	const string matName = luaL_checkstring(L, 1);
	const LuaMatType matType = ParseMaterialType(matName);

	if (!lua_istable(L, 2)) {
		luaL_error(L, "Incorrect arguments to GetMaterial");
	}

  LuaMatRef** matRef = (LuaMatRef**) lua_newuserdata(L, sizeof(LuaMatRef*));
  luaL_getmetatable(L, "MatRef");
  lua_setmetatable(L, -2);

	*matRef = SAFE_NEW LuaMatRef;

	**matRef = ParseMaterial(L, __FUNCTION__, 2, matType);

	return 1;
}


static int material_index(lua_State* L)
{
	LuaMatRef** matRef = (LuaMatRef**) luaL_checkudata(L, 1, "MatRef");
	const string key = luaL_checkstring(L, 2);
	const LuaMatBin* bin = (*matRef)->GetBin();
	if (bin == NULL) {
		return 0;
	}
	const LuaMaterial& mat = *bin;
	if (key == "type") {
	}
	return 0;
}


static int material_newindex(lua_State* L)
{
	LuaMatRef** matRef = (LuaMatRef**) luaL_checkudata(L, 1, "MatRef");
	return 0;
}


static int material_gc(lua_State* L)
{
	LuaMatRef** matRef = (LuaMatRef**) luaL_checkudata(L, 1, "MatRef");

	delete *matRef;

	return 0;
}


static void CreateMatRefMetatable(lua_State* L)
{
	luaL_newmetatable(L, "MatRef");
	HSTR_PUSH_CFUNC(L, "__gc",       material_gc);
	HSTR_PUSH_CFUNC(L, "__index",    material_index);
	HSTR_PUSH_CFUNC(L, "__newindex", material_newindex);
	lua_pop(L, 1);
}


/******************************************************************************/
/******************************************************************************/

int LuaUnitRendering::SetMaterial(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const unsigned int lod = (unsigned int)luaL_checknumber(L, 2) - 1;
	const string matName = luaL_checkstring(L, 3);
	const LuaMatType matType = ParseMaterialType(matName);
	LuaUnitMaterial* unitMat = GetUnitMaterial(unit, matName);
	if (unitMat == NULL) {
		return 0;
	}
	LuaUnitLODMaterial* lodMat = unitMat->GetMaterial(lod);
	if (lodMat == NULL) {
		return 0;
	}
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


int LuaUnitRendering::SetMaterialLastLOD(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const string matName = luaL_checkstring(L, 2);
	LuaUnitMaterial* unitMat = GetUnitMaterial(unit, matName);
	if (unitMat == NULL) {
		return 0;
	}
	const unsigned int lod = (unsigned int)luaL_checknumber(L, 3) - 1;
	unitMat->SetLastLOD(lod);
	return 0;
}


int LuaUnitRendering::SetMaterialDisplayLists(lua_State* L)
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const unsigned int lod = (unsigned int)luaL_checknumber(L, 2) - 1;
	const string matName = luaL_checkstring(L, 3);
	LuaUnitMaterial* unitMat = GetUnitMaterial(unit, matName);
	if (unitMat == NULL) {
		return 0;
	}
	LuaUnitLODMaterial* lodMat = unitMat->GetMaterial(lod);
	if (lodMat == NULL) {
		return 0;
	}
	lodMat->preDisplayList  = ParseDisplayList(L, __FUNCTION__, 4);
	lodMat->postDisplayList = ParseDisplayList(L, __FUNCTION__, 5);
	return 0;
}


// Set all lods at once?
int LuaUnitRendering::SetUnitUniform(lua_State* L) // FIXME
{
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	const string matName = luaL_checkstring(L, 2);
	LuaUnitMaterial* unitMat = GetUnitMaterial(unit, matName);
	if (unitMat == NULL) {
		return 0;
	}
	const unsigned int lod = (unsigned int)luaL_checknumber(L, 3) - 1;
	LuaUnitLODMaterial* lodMat = unitMat->GetMaterial(lod);
	if (lodMat == NULL) {
		return 0;
	}
	const int args = lua_gettop(L) - 3;
	const int lastArg = min(args, lodMat->uniforms.customCount + 3);
	for (int i = 3; i <= lastArg; i++) {

	}
	return 0;
}


/******************************************************************************/
/******************************************************************************/

int LuaUnitRendering::SetUnitLuaDraw(lua_State* L)
{
	const int unitID = (int)luaL_checknumber(L, 1);
	if ((unitID < 0) || (unitID >= MAX_UNITS)) {
		return 0;
	}
	CUnit* unit = uh->units[unitID];
	if (unit == NULL) {
		return 0;
	}

	if (!lua_isboolean(L, 2)) {
		return 0;
	}
	unit->luaDraw = lua_toboolean(L, 2);
	return 0;
}


/******************************************************************************/
/******************************************************************************/

static void PrintUnitLOD(const CUnit* unit, int lod)
{
	logOutput.Print("  LOD %i:\n", lod);
	logOutput.Print("    LodLength = %f\n", unit->lodLengths[lod]);
	for (int type = 0; type < LUAMAT_TYPE_COUNT; type++) {
		const LuaUnitMaterial& luaMat = unit->luaMats[type];
		const LuaUnitLODMaterial* lodMat = luaMat.GetMaterial(lod);
		const LuaMatBin* bin = lodMat->matref.GetBin();
		if (bin) {
			bin->Print("    ");
		}
	}
}


int LuaUnitRendering::Debug(lua_State* L)
{
	const int args = lua_gettop(L);
	if (args == 0) {
		luaMatHandler.PrintAllBins("");
		return 0;
	}
	CUnit* unit = ParseUnit(L, __FUNCTION__, 1);
	if (unit == NULL) {
		return 0;
	}
	logOutput.Print("\n");
	logOutput.Print("UnitID      = %i\n", unit->id);
	logOutput.Print("UnitDefID   = %i\n", unit->unitDef->id);
	logOutput.Print("UnitDefName = %s\n", unit->unitDef->name.c_str());
	logOutput.Print("LodCount    = %i\n", unit->lodCount);
	logOutput.Print("CurrentLod  = %i\n", unit->currentLOD);
	logOutput.Print("\n");

	const LuaUnitMaterial& alphaMat      = unit->luaMats[LUAMAT_ALPHA];
	const LuaUnitMaterial& opaqueMat     = unit->luaMats[LUAMAT_OPAQUE];
	const LuaUnitMaterial& alphaReflMat  = unit->luaMats[LUAMAT_ALPHA_REFLECT];
	const LuaUnitMaterial& opaqueReflMat = unit->luaMats[LUAMAT_OPAQUE_REFLECT];
	const LuaUnitMaterial& shadowMat     = unit->luaMats[LUAMAT_SHADOW];
	logOutput.Print("LUAMAT_ALPHA          lastLOD = %i\n", alphaMat.GetLastLOD());
	logOutput.Print("LUAMAT_OPAQUE         lastLOD = %i\n", opaqueMat.GetLastLOD());
	logOutput.Print("LUAMAT_ALPHA_REFLECT  lastLOD = %i\n", alphaReflMat.GetLastLOD());
	logOutput.Print("LUAMAT_OPAQUE_REFLECT lastLOD = %i\n", opaqueReflMat.GetLastLOD());
	logOutput.Print("LUAMAT_SHADOW         lastLOD = %i\n", shadowMat.GetLastLOD());

	for (int lod = 0; lod < unit->lodCount; lod++) {
		PrintUnitLOD(unit, lod);
	}

	return 0;
}

/******************************************************************************/
/******************************************************************************/

