/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include <set>
#include <string>
#include <vector>
#include <set>
#include <map>
#include <cctype>


#include "LuaUnitDefs.h"

#include "LuaInclude.h"

#include "LuaConfig.h"
#include "LuaDefs.h"
#include "LuaHandle.h"
#include "LuaUtils.h"
#include "Game/Game.h"
#include "Game/GameHelper.h"
#include "Sim/Misc/Team.h"
#include "Map/Ground.h"
#include "Map/MapDamage.h"
#include "Map/MapInfo.h"
#include "Rendering/IconHandler.h"
#include "Rendering/Models/IModelParser.h"
#include "Sim/Features/Feature.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Misc/CategoryHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/Wind.h"
#include "Sim/MoveTypes/MoveDefHandler.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/UnitHandler.h"
#include "Sim/Units/UnitDefHandler.h"
#include "Sim/Units/UnitDefImage.h"
#include "Sim/Units/UnitTypes/Builder.h"
#include "Sim/Units/UnitTypes/Factory.h"
#include "Sim/Units/CommandAI/Command.h"
#include "Sim/Units/CommandAI/CommandAI.h"
#include "Sim/Units/CommandAI/FactoryCAI.h"
#include "Sim/Weapons/Weapon.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/FileSystem/FileSystem.h"
#include "System/Log/ILog.h"
#include "System/Util.h"


static ParamMap paramMap;

static bool InitParamMap();

// iteration routines
static int Next(lua_State* L);
static int Pairs(lua_State* L);

// meta-table calls
static int UnitDefIndex(lua_State* L);
static int UnitDefNewIndex(lua_State* L);
static int UnitDefMetatable(lua_State* L);

// special access functions
static int UnitDefToID(lua_State* L, const void* data);
static int WeaponDefToID(lua_State* L, const void* data);
static int CustomParamsTable(lua_State* L, const void* data);
static int BuildOptions(lua_State* L, const void* data);
static int SoundsTable(lua_State* L, const void* data);
static int WeaponsTable(lua_State* L, const void* data);
static int CategorySetFromBits(lua_State* L, const void* data);
static int CategorySetFromString(lua_State* L, const void* data);


/******************************************************************************/
/******************************************************************************/

bool LuaUnitDefs::PushEntries(lua_State* L)
{
	InitParamMap();

	typedef int (*IndxFuncType)(lua_State*);
	typedef int (*IterFuncType)(lua_State*);
	typedef std::map<std::string, int> ObjectDefMapType;

	const ObjectDefMapType& defsMap = unitDefHandler->unitDefIDsByName;

	const char* indxOpers[] = {"__index", "__newindex", "__metatable"};
	const char* iterOpers[] = {"pairs", "next"};

	const IndxFuncType indxFuncs[] = {&UnitDefIndex, &UnitDefNewIndex, &UnitDefMetatable};
	const IterFuncType iterFuncs[] = {&Pairs, &Next};

	for (auto it = defsMap.cbegin(); it != defsMap.cend(); ++it) {
		const auto def = unitDefHandler->GetUnitDefByID(it->second);

		if (def == NULL)
			continue;

		PushObjectDefProxyTable(L, indxOpers, iterOpers, indxFuncs, iterFuncs, def);
	}

	return true;
}



/******************************************************************************/

static int UnitDefIndex(lua_State* L)
{
	// not a default value
	if (!lua_isstring(L, 2)) {
		lua_rawget(L, 1);
		return 1;
	}

	const char* name = lua_tostring(L, 2);
	ParamMap::const_iterator it = paramMap.find(name);

	// not a default value
	if (it == paramMap.end()) {
	  lua_rawget(L, 1);
	  return 1;
	}

	const void* userData = lua_touserdata(L, lua_upvalueindex(1));
	const UnitDef* ud = static_cast<const UnitDef*>(userData);
	const DataElement& elem = it->second;
	const void* p = ((const char*)ud) + elem.offset;
	switch (elem.type) {
		case READONLY_TYPE: {
			lua_rawget(L, 1);
			return 1;
		}
		case INT_TYPE: {
			lua_pushnumber(L, *reinterpret_cast<const int*>(p));
			return 1;
		}
		case BOOL_TYPE: {
			lua_pushboolean(L, *reinterpret_cast<const bool*>(p));
			return 1;
		}
		case FLOAT_TYPE: {
			lua_pushnumber(L, *reinterpret_cast<const float*>(p));
			return 1;
		}
		case STRING_TYPE: {
			lua_pushsstring(L, *reinterpret_cast<const std::string*>(p));
			return 1;
		}
		case FUNCTION_TYPE: {
			return elem.func(L, p);
		}
		case ERROR_TYPE: {
			LOG_L(L_ERROR, "[%s] ERROR_TYPE for key \"%s\" in UnitDefs __index", __FUNCTION__, name);
			lua_pushnil(L);
			return 1;
		}
	}

	return 0;
}


static int UnitDefNewIndex(lua_State* L)
{
	// not a default value, set it
	if (!lua_isstring(L, 2)) {
		lua_rawset(L, 1);
		return 0;
	}

	const char* name = lua_tostring(L, 2);
	ParamMap::const_iterator it = paramMap.find(name);

	// not a default value, set it
	if (it == paramMap.end()) {
		lua_rawset(L, 1);
		return 0;
	}

	const void* userData = lua_touserdata(L, lua_upvalueindex(1));
	const UnitDef* ud = static_cast<const UnitDef*>(userData);

	// write-protected
	if (!gs->editDefsEnabled) {
		luaL_error(L, "Attempt to write UnitDefs[%d].%s", ud->id, name);
		return 0;
	}

	// Definition editing
	const DataElement& elem = it->second;
	const void* p = ((const char*)ud) + elem.offset;

	switch (elem.type) {
		case FUNCTION_TYPE:
		case READONLY_TYPE: {
			luaL_error(L, "Can not write to %s", name);
			return 0;
		}
		case INT_TYPE: {
			*((int*)p) = lua_toint(L, -1);
			return 0;
		}
		case BOOL_TYPE: {
			*((bool*)p) = lua_toboolean(L, -1);
			return 0;
		}
		case FLOAT_TYPE: {
			*((float*)p) = lua_tofloat(L, -1);
			return 0;
		}
		case STRING_TYPE: {
			*((string*)p) = lua_tostring(L, -1);
			return 0;
		}
		case ERROR_TYPE: {
			LOG_L(L_ERROR, "[%s] ERROR_TYPE for key \"%s\" in UnitDefs __newindex", __FUNCTION__, name);
			lua_pushnil(L);
			return 1;
		}
	}

	return 0;
}


static int UnitDefMetatable(lua_State* L)
{
	lua_touserdata(L, lua_upvalueindex(1));
	// const void* userData = lua_touserdata(L, lua_upvalueindex(1));
	// const UnitDef* ud = (const UnitDef*)userData;
	return 0;
}


/******************************************************************************/

static int Next(lua_State* L)
{
	return LuaUtils::Next(paramMap, L);
}


static int Pairs(lua_State* L)
{
	luaL_checktype(L, 1, LUA_TTABLE);
	lua_pushcfunction(L, Next);	// iterator
	lua_pushvalue(L, 1);        // state (table)
	lua_pushnil(L);             // initial value
	return 3;
}


/******************************************************************************/
/******************************************************************************/

static int UnitDefToID(lua_State* L, const void* data)
{
	const UnitDef* ud = *((const UnitDef**)data);
	if (ud == NULL) {
		return 0;
	}
	lua_pushnumber(L, ud->id);
	return 1;
}


static int WeaponDefToID(lua_State* L, const void* data)
{
	const WeaponDef* wd = *((const WeaponDef**)data);
	if (wd == NULL) {
		return 0;
	}
	lua_pushnumber(L, wd->id);
	return 1;
}


static int WeaponDefToName(lua_State* L, const void* data)
{
	const WeaponDef* wd = *((const WeaponDef**)data);
	if (wd == NULL) {
		return 0;
	}
	lua_pushsstring(L, wd->name);
	return 1;
}


static int SafeIconType(lua_State* L, const void* data)
{
	// the iconType is unsynced because LuaUI has SetUnitDefIcon()
	if (!CLuaHandle::GetHandleSynced(L)) {
		const icon::CIcon& iconType = *((const icon::CIcon*)data);
		lua_pushsstring(L, iconType->GetName());
		return 1;
	}
	return 0;
}


static int CustomParamsTable(lua_State* L, const void* data)
{
	const map<string, string>& params = *((const map<string, string>*)data);
	lua_newtable(L);
	map<string, string>::const_iterator it;
	for (it = params.begin(); it != params.end(); ++it) {
		lua_pushsstring(L, it->first);
		lua_pushsstring(L, it->second);
		lua_rawset(L, -3);
	}
	return 1;
}


static int BuildOptions(lua_State* L, const void* data)
{
	const map<int, string>& buildOptions = *((const map<int, string>*)data);
	const map<string, int>& unitMap = unitDefHandler->unitDefIDsByName;

	lua_newtable(L);
	int count = 0;
	map<int, string>::const_iterator it;
	for (it = buildOptions.begin(); it != buildOptions.end(); ++it) {
		map<string, int>::const_iterator fit = unitMap.find(it->second);
		if (fit != unitMap.end()) {
			count++;
			lua_pushnumber(L, count);
			lua_pushnumber(L, fit->second); // UnitDef id
			lua_rawset(L, -3);
		}
	}
	return 1;
}


static inline int BuildCategorySet(lua_State* L, const vector<string>& cats)
{
	lua_newtable(L);
	const int count = (int)cats.size();
	for (int i = 0; i < count; i++) {
		lua_pushsstring(L, cats[i]);
		lua_pushboolean(L, true);
		lua_rawset(L, -3);
	}
	return 1;
}


static int CategorySetFromBits(lua_State* L, const void* data)
{
	const int bits = *((const int*)data);
	const vector<string> &cats =
		CCategoryHandler::Instance()->GetCategoryNames(bits);
	return BuildCategorySet(L, cats);
}


static int CategorySetFromString(lua_State* L, const void* data)
{
	const string& str = *((const string*)data);
	const string lower = StringToLower(str);
	const vector<string> &cats = CSimpleParser::Tokenize(lower, 0);
	return BuildCategorySet(L, cats);
}


static int WeaponsTable(lua_State* L, const void* data)
{
	const vector<UnitDefWeapon>& weapons =
		*((const vector<UnitDefWeapon>*)data);

	lua_newtable(L);

	for (size_t i = 0; i < weapons.size(); i++) {
		const UnitDefWeapon& udw = weapons[i];
		const WeaponDef* weapon = udw.def;
		lua_pushnumber(L, i + LUA_WEAPON_BASE_INDEX);
		lua_newtable(L); {
			HSTR_PUSH_NUMBER(L, "weaponDef",   weapon->id);
			HSTR_PUSH_NUMBER(L, "slavedTo",    udw.slavedTo-1+LUA_WEAPON_BASE_INDEX);
			HSTR_PUSH_NUMBER(L, "fuelUsage",   udw.fuelUsage);
			HSTR_PUSH_NUMBER(L, "maxAngleDif", udw.maxMainDirAngleDif);
			HSTR_PUSH_NUMBER(L, "mainDirX",    udw.mainDir.x);
			HSTR_PUSH_NUMBER(L, "mainDirY",    udw.mainDir.y);
			HSTR_PUSH_NUMBER(L, "mainDirZ",    udw.mainDir.z);

			HSTR_PUSH(L, "badTargets");
			CategorySetFromBits(L, &udw.badTargetCat);
			lua_rawset(L, -3);

			HSTR_PUSH(L, "onlyTargets");
			CategorySetFromBits(L, &udw.onlyTargetCat);
			lua_rawset(L, -3);
		}
		lua_rawset(L, -3);
	}

	return 1;
}


static void PushGuiSoundSet(lua_State* L, const string& name,
                            const GuiSoundSet& soundSet)
{
	const int soundCount = (int)soundSet.sounds.size();
	lua_pushsstring(L, name);
	lua_newtable(L);
	for (int i = 0; i < soundCount; i++) {
		lua_pushnumber(L, i + 1);
		lua_newtable(L);
		const GuiSoundSet::Data& sound = soundSet.sounds[i];
		HSTR_PUSH_STRING(L, "name",   sound.name);
		HSTR_PUSH_NUMBER(L, "volume", sound.volume);
		if (!CLuaHandle::GetHandleSynced(L)) {
			HSTR_PUSH_NUMBER(L, "id", sound.id);
		}
		lua_rawset(L, -3);
	}
	lua_rawset(L, -3);
}


static int SoundsTable(lua_State* L, const void* data) {
	const UnitDef::SoundStruct& sounds = *((const UnitDef::SoundStruct*) data);

	lua_newtable(L);
	PushGuiSoundSet(L, "select",      sounds.select);
	PushGuiSoundSet(L, "ok",          sounds.ok);
	PushGuiSoundSet(L, "arrived",     sounds.arrived);
	PushGuiSoundSet(L, "build",       sounds.build);
	PushGuiSoundSet(L, "repair",      sounds.repair);
	PushGuiSoundSet(L, "working",     sounds.working);
	PushGuiSoundSet(L, "underattack", sounds.underattack);
	PushGuiSoundSet(L, "cant",        sounds.cant);
	PushGuiSoundSet(L, "activate",    sounds.activate);
	PushGuiSoundSet(L, "deactivate",  sounds.deactivate);

	return 1;
}




static int MoveDefTable(lua_State* L, const void* data)
{
	const unsigned int mdType = *static_cast<const unsigned int*>(data);
	const MoveDef* md = NULL;

	lua_newtable(L);
	if (mdType == -1U) {
		return 1;
	}
	if ((md = moveDefHandler->GetMoveDefByPathType(mdType)) == NULL) {
		return 1;
	}

	HSTR_PUSH_NUMBER(L, "id", md->pathType);

	switch (md->speedModClass) {
		case MoveDef::Tank:  { HSTR_PUSH_STRING(L, "family", "tank");  HSTR_PUSH_STRING(L, "type", "ground"); break; }
		case MoveDef::KBot:  { HSTR_PUSH_STRING(L, "family", "kbot");  HSTR_PUSH_STRING(L, "type", "ground"); break; }
		case MoveDef::Hover: { HSTR_PUSH_STRING(L, "family", "hover"); HSTR_PUSH_STRING(L, "type",  "hover"); break; }
		case MoveDef::Ship:  { HSTR_PUSH_STRING(L, "family", "ship");  HSTR_PUSH_STRING(L, "type",   "ship"); break; }
	}

	HSTR_PUSH_NUMBER(L, "xsize",         md->xsize);
	HSTR_PUSH_NUMBER(L, "zsize",         md->zsize);
	HSTR_PUSH_NUMBER(L, "depth",         md->depth);
	HSTR_PUSH_NUMBER(L, "maxSlope",      md->maxSlope);
	HSTR_PUSH_NUMBER(L, "slopeMod",      md->slopeMod);
	HSTR_PUSH_NUMBER(L, "depthMod",      md->depthModParams[MoveDef::DEPTHMOD_LIN_COEFF]);
	HSTR_PUSH_NUMBER(L, "crushStrength", md->crushStrength);

	HSTR_PUSH_BOOL(L, "heatMapping",     md->heatMapping);
	HSTR_PUSH_NUMBER(L, "heatMod",       md->heatMod);
	HSTR_PUSH_NUMBER(L, "heatProduced",  md->heatProduced);

	HSTR_PUSH_STRING(L, "name", md->name);

	return 1;
}


static int TotalEnergyOut(lua_State* L, const void* data)
{
	const UnitDef& ud = *static_cast<const UnitDef*>(data);
	const float basicEnergy = (ud.energyMake - ud.energyUpkeep);
	const float tidalEnergy = (ud.tidalGenerator * mapInfo->map.tidalStrength);
	float windEnergy = 0.0f;
	if (ud.windGenerator > 0.0f) {
		windEnergy = (0.25f * (wind.GetMinWind() + wind.GetMaxWind()));
	}
	lua_pushnumber(L, basicEnergy + tidalEnergy + windEnergy); // CUSTOM
	return 1;
}



static int ModelTable(lua_State* L, const void* data) {
	const UnitDef* ud = static_cast<const UnitDef*>(data);
	const std::string modelFile = modelParser->FindModelPath(ud->modelName);

	lua_newtable(L);
	HSTR_PUSH_STRING(L, "type", StringToLower(FileSystem::GetExtension(modelFile)));
	HSTR_PUSH_STRING(L, "path", modelFile);
	HSTR_PUSH_STRING(L, "name", ud->modelName);
	HSTR_PUSH(L, "textures");

	lua_newtable(L);
	if (ud->model != NULL) {
		LuaPushNamedString(L, "tex1", ud->model->tex1);
		LuaPushNamedString(L, "tex2", ud->model->tex2);
	}
	lua_rawset(L, -3);
	return 1;
}


static int ColVolTable(lua_State* L, const void* data) {
	auto cv = static_cast<const CollisionVolume*>(data);
	assert(cv != NULL);

	lua_newtable(L);
	switch (cv->GetVolumeType()) {
		case CollisionVolume::COLVOL_TYPE_ELLIPSOID:
			HSTR_PUSH_STRING(L, "type", "ellipsoid");
			break;
		case CollisionVolume::COLVOL_TYPE_CYLINDER:
			HSTR_PUSH_STRING(L, "type", "cylinder");
			break;
		case CollisionVolume::COLVOL_TYPE_BOX:
			HSTR_PUSH_STRING(L, "type", "box");
			break;
		case CollisionVolume::COLVOL_TYPE_SPHERE:
			HSTR_PUSH_STRING(L, "type", "sphere");
			break;
	}

	LuaPushNamedNumber(L, "scaleX", cv->GetScales().x);
	LuaPushNamedNumber(L, "scaleY", cv->GetScales().y);
	LuaPushNamedNumber(L, "scaleZ", cv->GetScales().z);
	LuaPushNamedNumber(L, "offsetX", cv->GetOffsets().x);
	LuaPushNamedNumber(L, "offsetY", cv->GetOffsets().y);
	LuaPushNamedNumber(L, "offsetZ", cv->GetOffsets().z);
	LuaPushNamedNumber(L, "boundingRadius", cv->GetBoundingRadius());
	LuaPushNamedBool(L, "defaultToSphere",    cv->DefaultToSphere());
	LuaPushNamedBool(L, "defaultToFootPrint", cv->DefaultToFootPrint());
	LuaPushNamedBool(L, "defaultToPieceTree", cv->DefaultToPieceTree());
	return 1;
}


#define TYPE_FUNC(FuncName, LuaType)                           \
	static int FuncName(lua_State* L, const void* data)        \
	{                                                          \
		const UnitDef* ud = static_cast<const UnitDef*>(data); \
		lua_push ## LuaType(L, ud->FuncName());                \
		return 1;                                              \
	}

#define TYPE_MODEL_FUNC(name, param)                           \
	static int name(lua_State* L, const void* data)            \
	{                                                          \
		const UnitDef* ud = static_cast<const UnitDef*>(data); \
		const S3DModel* model = ud->LoadModel();               \
		lua_pushnumber(L, model->param);                       \
		return 1;                                              \
	}

TYPE_FUNC(IsTransportUnit, boolean)
TYPE_FUNC(IsImmobileUnit, boolean)
TYPE_FUNC(IsBuildingUnit, boolean)
TYPE_FUNC(IsBuilderUnit, boolean)
TYPE_FUNC(IsMobileBuilderUnit, boolean)
TYPE_FUNC(IsStaticBuilderUnit, boolean)
TYPE_FUNC(IsFactoryUnit, boolean)
TYPE_FUNC(IsExtractorUnit, boolean)
TYPE_FUNC(IsGroundUnit, boolean)
TYPE_FUNC(IsAirUnit, boolean)
TYPE_FUNC(IsStrafingAirUnit, boolean)
TYPE_FUNC(IsHoveringAirUnit, boolean)
TYPE_FUNC(IsFighterAirUnit, boolean)
TYPE_FUNC(IsBomberAirUnit, boolean)

TYPE_MODEL_FUNC(ModelHeight, height)
TYPE_MODEL_FUNC(ModelRadius, radius)
TYPE_MODEL_FUNC(ModelMinx,   mins.x)
TYPE_MODEL_FUNC(ModelMidx,   relMidPos.x)
TYPE_MODEL_FUNC(ModelMaxx,   maxs.x)
TYPE_MODEL_FUNC(ModelMiny,   mins.y)
TYPE_MODEL_FUNC(ModelMidy,   relMidPos.y)
TYPE_MODEL_FUNC(ModelMaxy,   maxs.y)
TYPE_MODEL_FUNC(ModelMinz,   mins.z)
TYPE_MODEL_FUNC(ModelMidz,   relMidPos.z)
TYPE_MODEL_FUNC(ModelMaxz,   maxs.z)



static int ReturnEmptyString(lua_State* L, const void* data) {
	LOG_L(L_WARNING, "[%s] %s - deprecated field!", __FUNCTION__, lua_tostring(L, 2));
	lua_pushstring(L, "");
	return 1;
}

static int ReturnFalse(lua_State* L, const void* data) {
	LOG_L(L_WARNING, "[%s] %s - deprecated field!", __FUNCTION__, lua_tostring(L, 2));
	lua_pushboolean(L, false);
	return 1;
}

static int ReturnMinusOne(lua_State* L, const void* data) {
	LOG_L(L_WARNING, "[%s] %s - deprecated field!", __FUNCTION__, lua_tostring(L, 2));
	lua_pushnumber(L, -1);
	return 1;
}

static int ReturnNil(lua_State* L, const void* data) {
	LOG_L(L_WARNING, "[%s] %s - deprecated field!", __FUNCTION__, lua_tostring(L, 2));
	lua_pushnil(L);
	return 1;
}

/******************************************************************************/
/******************************************************************************/

static bool InitParamMap()
{
	paramMap.clear();

	paramMap["next"]  = DataElement(READONLY_TYPE);
	paramMap["pairs"] = DataElement(READONLY_TYPE);

	// dummy UnitDef for address lookups
	const UnitDef& ud = *unitDefHandler->unitDefs[0];
	const char* start = ADDRESS(ud);

/*
ADD_FLOAT("maxRange",       maxRange);       // CUSTOM
ADD_BOOL("hasShield",       hasShield);      // CUSTOM
ADD_BOOL("canParalyze",     canParalyze);    // CUSTOM
ADD_BOOL("canStockpile",    canStockpile);   // CUSTOM
ADD_BOOL("canAttackWater",  canAttackWater); // CUSTOM
*/
// ADD_INT("buildOptionsCount", ud.buildOptions.size(")); // CUSTOM

	ADD_FUNCTION("builder", ud, ReturnFalse); // DEPRECATED
	ADD_FUNCTION("floater", ud, ReturnFalse); // DEPRECATED
	ADD_FUNCTION("canDGun", ud, ReturnFalse); // DEPRECATED
	ADD_FUNCTION("canCrash", ud, ReturnFalse); // DEPRECATED
	ADD_FUNCTION("isCommander", ud, ReturnFalse); // DEPRECATED
	ADD_FUNCTION("moveData", ud.pathType, ReturnNil); // DEPRECATED
	ADD_FUNCTION("type", ud, ReturnEmptyString); // DEPRECATED
	ADD_FUNCTION("maxSlope", ud, ReturnMinusOne); // DEPRECATED

	ADD_FUNCTION("totalEnergyOut", ud, TotalEnergyOut);

	ADD_FUNCTION("modCategories",      ud.categoryString,  CategorySetFromString);
	ADD_FUNCTION("springCategories",   ud.category,        CategorySetFromBits);
	ADD_FUNCTION("noChaseCategories",  ud.noChaseCategory, CategorySetFromBits);

	ADD_FUNCTION("customParams",       ud.customParams,       CustomParamsTable);
	ADD_FUNCTION("buildOptions",       ud.buildOptions,       BuildOptions);
	ADD_FUNCTION("decoyDef",           ud.decoyDef,           UnitDefToID);
	ADD_FUNCTION("weapons",            ud.weapons,            WeaponsTable);
	ADD_FUNCTION("sounds",             ud.sounds,             SoundsTable);
	ADD_FUNCTION("model",              ud,                    ModelTable);
	ADD_FUNCTION("moveDef",            ud.pathType,           MoveDefTable);
	ADD_FUNCTION("shieldWeaponDef",    ud.shieldWeaponDef,    WeaponDefToID);
	ADD_FUNCTION("stockpileWeaponDef", ud.stockpileWeaponDef, WeaponDefToID);
	ADD_FUNCTION("iconType",           ud.iconType,           SafeIconType);
	ADD_FUNCTION("collisionVolume",    ud.collisionVolume,    ColVolTable);

	ADD_FUNCTION("isTransport", ud, IsTransportUnit);
	ADD_FUNCTION("isImmobile", ud, IsImmobileUnit);
	ADD_FUNCTION("isBuilding", ud, IsBuildingUnit);
	ADD_FUNCTION("isBuilder", ud, IsBuilderUnit);
	ADD_FUNCTION("isMobileBuilder", ud, IsMobileBuilderUnit);
	ADD_FUNCTION("isStaticBuilder", ud, IsStaticBuilderUnit);
	ADD_FUNCTION("isFactory", ud, IsFactoryUnit);
	ADD_FUNCTION("isExtractor", ud, IsExtractorUnit);
	ADD_FUNCTION("isGroundUnit", ud, IsGroundUnit);
	ADD_FUNCTION("isAirUnit", ud, IsAirUnit);
	ADD_FUNCTION("isStrafingAirUnit", ud, IsStrafingAirUnit);
	ADD_FUNCTION("isHoveringAirUnit", ud, IsHoveringAirUnit);
	ADD_FUNCTION("isFighterAirUnit", ud, IsFighterAirUnit);
	ADD_FUNCTION("isBomberAirUnit", ud, IsBomberAirUnit);

	ADD_FUNCTION("height",  ud, ModelHeight);
	ADD_FUNCTION("radius",  ud, ModelRadius);
	ADD_FUNCTION("minx",    ud, ModelMinx);
	ADD_FUNCTION("midx",    ud, ModelMidx);
	ADD_FUNCTION("maxx",    ud, ModelMaxx);
	ADD_FUNCTION("miny",    ud, ModelMiny);
	ADD_FUNCTION("midy",    ud, ModelMidy);
	ADD_FUNCTION("maxy",    ud, ModelMaxy);
	ADD_FUNCTION("minz",    ud, ModelMinz);
	ADD_FUNCTION("midz",    ud, ModelMidz);
	ADD_FUNCTION("maxz",    ud, ModelMaxz);



	ADD_INT("id", ud.id);
	ADD_INT("cobID", ud.cobID);

	ADD_STRING("name",      ud.name);
	ADD_STRING("humanName", ud.humanName);

	ADD_STRING("tooltip", ud.tooltip);

	ADD_STRING("wreckName", ud.wreckName);

	ADD_FUNCTION("deathExplosion", ud.deathExpWeaponDef, WeaponDefToName);
	ADD_FUNCTION("selfDExplosion", ud.selfdExpWeaponDef, WeaponDefToName);

	ADD_STRING("buildpicname", ud.buildPicName);

	ADD_INT("techLevel",   ud.techLevel);
	ADD_INT("maxThisUnit", ud.maxThisUnit);

	ADD_FLOAT("metalUpkeep",    ud.metalUpkeep);
	ADD_FLOAT("energyUpkeep",   ud.energyUpkeep);
	ADD_FLOAT("metalMake",      ud.metalMake);
	ADD_FLOAT("makesMetal",     ud.makesMetal);
	ADD_FLOAT("energyMake",     ud.energyMake);
	ADD_FLOAT("metalCost",      ud.metal);
	ADD_FLOAT("energyCost",     ud.energy);
	ADD_FLOAT("buildTime",      ud.buildTime);
	ADD_FLOAT("extractsMetal",  ud.extractsMetal);
	ADD_FLOAT("extractRange",   ud.extractRange);
	ADD_FLOAT("windGenerator",  ud.windGenerator);
	ADD_FLOAT("tidalGenerator", ud.tidalGenerator);
	ADD_FLOAT("metalStorage",   ud.metalStorage);
	ADD_FLOAT("energyStorage",  ud.energyStorage);

	ADD_DEPRECATED_LUADEF_KEY("extractSquare");

	ADD_FLOAT("power", ud.power);

	ADD_FLOAT("health",       ud.health);
	ADD_FLOAT("autoHeal",     ud.autoHeal);
	ADD_FLOAT("idleAutoHeal", ud.idleAutoHeal);

	ADD_INT("idleTime", ud.idleTime);

	ADD_BOOL("canSelfD", ud.canSelfD);
	ADD_INT("selfDCountdown", ud.selfDCountdown);

	ADD_FLOAT("speed",    ud.speed);
	ADD_FLOAT("rSpeed",    ud.rSpeed);
	ADD_FLOAT("turnRate", ud.turnRate);
	ADD_BOOL("turnInPlace", ud.turnInPlace);
	ADD_FLOAT("turnInPlaceSpeedLimit", ud.turnInPlaceSpeedLimit);

	ADD_BOOL("upright", ud.upright);
	ADD_BOOL("collide", ud.collide);

	ADD_FLOAT("losHeight",     ud.losHeight);
	ADD_FLOAT("losRadius",     ud.losRadius);
	ADD_FLOAT("airLosRadius",  ud.airLosRadius);

	ADD_INT("radarRadius",    ud.radarRadius);
	ADD_INT("sonarRadius",    ud.sonarRadius);
	ADD_INT("jammerRadius",   ud.jammerRadius);
	ADD_INT("sonarJamRadius", ud.sonarJamRadius);
	ADD_INT("seismicRadius",  ud.seismicRadius);

	ADD_FLOAT("seismicSignature", ud.seismicSignature);

	ADD_BOOL("stealth",      ud.stealth);
	ADD_BOOL("sonarStealth", ud.sonarStealth);

	ADD_FLOAT("mass", ud.mass);

	ADD_FLOAT("maxHeightDif",  ud.maxHeightDif);
	ADD_FLOAT("minWaterDepth", ud.minWaterDepth);
	ADD_FLOAT("maxWaterDepth", ud.maxWaterDepth);
	ADD_FLOAT("waterline",     ud.waterline);

	ADD_INT("flankingBonusMode",   ud.flankingBonusMode);
	ADD_FLOAT("flankingBonusMax",  ud.flankingBonusMax);
	ADD_FLOAT("flankingBonusMin",  ud.flankingBonusMin);
	ADD_FLOAT("flankingBonusDirX", ud.flankingBonusDir.x);
	ADD_FLOAT("flankingBonusDirY", ud.flankingBonusDir.y);
	ADD_FLOAT("flankingBonusDirZ", ud.flankingBonusDir.z);
	ADD_FLOAT("flankingBonusMobilityAdd", ud.flankingBonusMobilityAdd);

	ADD_INT("armorType",         ud.armorType);
	ADD_FLOAT("armoredMultiple", ud.armoredMultiple);

	ADD_FLOAT("minCollisionSpeed", ud.minCollisionSpeed);
	ADD_FLOAT("slideTolerance",    ud.slideTolerance);

	ADD_FLOAT("maxWeaponRange", ud.maxWeaponRange);
	ADD_FLOAT("maxCoverage", ud.maxCoverage);

	ADD_BOOL( "buildRange3D",   ud.buildRange3D);
	ADD_FLOAT("buildDistance",  ud.buildDistance);
	ADD_FLOAT("buildSpeed",     ud.buildSpeed);
	ADD_FLOAT("repairSpeed",    ud.repairSpeed);
	ADD_FLOAT("maxRepairSpeed", ud.repairSpeed);
	ADD_FLOAT("reclaimSpeed",   ud.reclaimSpeed);
	ADD_FLOAT("resurrectSpeed", ud.resurrectSpeed);
	ADD_FLOAT("captureSpeed",   ud.captureSpeed);
	ADD_FLOAT("terraformSpeed", ud.terraformSpeed);

	ADD_BOOL("canSubmerge",       ud.canSubmerge);
	ADD_BOOL("floatOnWater",      ud.floatOnWater);
	ADD_BOOL("canFly",            ud.canfly);
	ADD_BOOL("canMove",           ud.canmove);
	ADD_BOOL("onOffable",         ud.onoffable);
	ADD_BOOL("activateWhenBuilt", ud.activateWhenBuilt);

	ADD_DEPRECATED_LUADEF_KEY("canHover");

	ADD_BOOL("reclaimable", ud.reclaimable);
	ADD_BOOL("capturable",  ud.capturable);
	ADD_BOOL("repairable",  ud.repairable);

	ADD_BOOL("canManualFire",         ud.canManualFire);
	ADD_BOOL("canCloak",              ud.canCloak);
	ADD_BOOL("canRestore",            ud.canRestore);
	ADD_BOOL("canRepair",             ud.canRepair);
	ADD_BOOL("canSelfRepair",         ud.canSelfRepair);
	ADD_BOOL("canReclaim",            ud.canReclaim);
	ADD_BOOL("canAttack",             ud.canAttack);
	ADD_BOOL("canPatrol",             ud.canPatrol);
	ADD_BOOL("canFight",              ud.canFight);
	ADD_BOOL("canGuard",              ud.canGuard);
	ADD_BOOL("canAssist",             ud.canAssist);
	ADD_BOOL("canBeAssisted",         ud.canBeAssisted);
	ADD_BOOL("canRepeat",             ud.canRepeat);
	ADD_BOOL("canCapture",            ud.canCapture);
	ADD_BOOL("canResurrect",          ud.canResurrect);
	ADD_BOOL("canLoopbackAttack",     ud.canLoopbackAttack);
	ADD_BOOL("canFireControl",        ud.canFireControl);
	ADD_INT( "fireState",             ud.fireState);
	ADD_INT( "moveState",             ud.moveState);
	ADD_BOOL("fullHealthFactory",     ud.fullHealthFactory);
	ADD_BOOL("factoryHeadingTakeoff", ud.factoryHeadingTakeoff);

	//aircraft stuff
	ADD_DEPRECATED_LUADEF_KEY("drag");
	ADD_FLOAT("wingDrag",     ud.wingDrag);
	ADD_FLOAT("wingAngle",    ud.wingAngle);
	ADD_FLOAT("crashDrag",    ud.crashDrag);
	ADD_FLOAT("frontToSpeed", ud.frontToSpeed);
	ADD_FLOAT("speedToFront", ud.speedToFront);
	ADD_FLOAT("myGravity",    ud.myGravity);
	ADD_FLOAT("verticalSpeed",ud.verticalSpeed);

	ADD_FLOAT("maxBank",      ud.maxBank);
	ADD_FLOAT("maxPitch",     ud.maxPitch);
	ADD_FLOAT("turnRadius",   ud.turnRadius);
	ADD_FLOAT("wantedHeight", ud.wantedHeight);
	ADD_BOOL("hoverAttack",   ud.hoverAttack);
	ADD_BOOL("airStrafe",     ud.airStrafe);
	ADD_BOOL("bankingAllowed",ud.bankingAllowed);
	ADD_BOOL("useSmoothMesh", ud.useSmoothMesh);

	// < 0 means it can land,
	// >= 0 indicates how much the unit will move during hovering on the spot
	ADD_FLOAT("dlHoverFactor", ud.dlHoverFactor);

//	bool DontLand (") { return dlHoverFactor >= 0.0f; }

	ADD_FLOAT("maxAcc",      ud.maxAcc);
	ADD_FLOAT("maxDec",      ud.maxDec);
	ADD_FLOAT("maxAileron",  ud.maxAileron);
	ADD_FLOAT("maxElevator", ud.maxElevator);
	ADD_FLOAT("maxRudder",   ud.maxRudder);
	ADD_FLOAT("attackSafetyDistance", ud.attackSafetyDistance);

	ADD_FLOAT("maxFuel",    ud.maxFuel);
	ADD_FLOAT("refuelTime", ud.refuelTime);

	ADD_FLOAT("minAirBasePower", ud.minAirBasePower);

//	unsigned char* yardmapLevels[6];
//	unsigned char* yardmaps[4];			//Iterations of the Ymap for building rotation

	ADD_INT("xsize", ud.xsize);
	ADD_INT("zsize", ud.zsize);

	// transport stuff
	ADD_INT(  "transportCapacity",     ud.transportCapacity);
	ADD_INT(  "transportSize",         ud.transportSize);
	ADD_FLOAT("transportMass",         ud.transportMass);
	ADD_FLOAT("loadingRadius",         ud.loadingRadius);
	ADD_BOOL( "isAirBase",             ud.isAirBase);
	ADD_BOOL( "isFirePlatform",        ud.isFirePlatform);
	ADD_BOOL( "holdSteady",            ud.holdSteady);
	ADD_BOOL( "releaseHeld",           ud.releaseHeld);
	ADD_BOOL( "cantBeTransported",     ud.cantBeTransported);
	ADD_BOOL( "transportByEnemy",      ud.transportByEnemy);
	ADD_INT(  "transportUnloadMethod", ud.transportUnloadMethod);
	ADD_FLOAT("fallSpeed",             ud.fallSpeed);
	ADD_FLOAT("unitFallSpeed",         ud.unitFallSpeed);

	ADD_BOOL( "startCloaked",     ud.startCloaked);
	ADD_FLOAT("cloakCost",        ud.cloakCost);
	ADD_FLOAT("cloakCostMoving",  ud.cloakCostMoving);
	ADD_FLOAT("decloakDistance",  ud.decloakDistance);
	ADD_BOOL( "decloakSpherical", ud.decloakSpherical);
	ADD_BOOL( "decloakOnFire",    ud.decloakOnFire);
	ADD_INT(  "cloakTimeout",     ud.cloakTimeout);

	ADD_BOOL( "canKamikaze",    ud.canKamikaze);
	ADD_FLOAT("kamikazeDist",   ud.kamikazeDist);
	ADD_BOOL( "kamikazeUseLOS", ud.kamikazeUseLOS);

	ADD_BOOL("targfac", ud.targfac);

	ADD_BOOL("needGeo",   ud.needGeo);
	ADD_BOOL("isFeature", ud.isFeature);

	ADD_BOOL("hideDamage",     ud.hideDamage);
	ADD_BOOL("showPlayerName", ud.showPlayerName);

	ADD_INT("highTrajectoryType", ud.highTrajectoryType);

	ADD_BOOL( "leaveTracks",   ud.decalDef.leaveTrackDecals);
	ADD_INT(  "trackType",     ud.decalDef.trackDecalType);
	ADD_FLOAT("trackWidth",    ud.decalDef.trackDecalWidth);
	ADD_FLOAT("trackOffset",   ud.decalDef.trackDecalOffset);
	ADD_FLOAT("trackStrength", ud.decalDef.trackDecalStrength);
	ADD_FLOAT("trackStretch",  ud.decalDef.trackDecalStretch);

	ADD_BOOL( "canDropFlare",     ud.canDropFlare);
	ADD_FLOAT("flareReloadTime",  ud.flareReloadTime);
	ADD_FLOAT("flareEfficiency",  ud.flareEfficiency);
	ADD_FLOAT("flareDelay",       ud.flareDelay);
	ADD_FLOAT("flareDropVectorX", ud.flareDropVector.x);
	ADD_FLOAT("flareDropVectorY", ud.flareDropVector.y);
	ADD_FLOAT("flareDropVectorZ", ud.flareDropVector.z);
	ADD_INT(  "flareTime",        ud.flareTime);
	ADD_INT(  "flareSalvoSize",   ud.flareSalvoSize);
	ADD_INT(  "flareSalvoDelay",  ud.flareSalvoDelay);

	ADD_BOOL("levelGround", ud.levelGround);
	ADD_BOOL("strafeToAttack", ud.strafeToAttack);

	ADD_BOOL( "useBuildingGroundDecal",  ud.decalDef.useGroundDecal);
	ADD_INT(  "buildingDecalType",       ud.decalDef.groundDecalType);
	ADD_INT(  "buildingDecalSizeX",      ud.decalDef.groundDecalSizeX);
	ADD_INT(  "buildingDecalSizeY",      ud.decalDef.groundDecalSizeY);
	ADD_FLOAT("buildingDecalDecaySpeed", ud.decalDef.groundDecalDecaySpeed);

	ADD_BOOL("showNanoFrame", ud.showNanoFrame);
	ADD_BOOL("showNanoSpray", ud.showNanoSpray);
	ADD_FLOAT("nanoColorR",   ud.nanoColor.x);
	ADD_FLOAT("nanoColorG",   ud.nanoColor.y);
	ADD_FLOAT("nanoColorB",   ud.nanoColor.z);

	ADD_STRING("scriptName", ud.scriptName);
	ADD_STRING("scriptPath", ud.scriptName); //FIXME // backward compability

	return true;
}


/******************************************************************************/
/******************************************************************************/
