/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "MapInfo.h"

#include "Sim/Misc/GlobalConstants.h"
#include "Rendering/GlobalRendering.h"

#include "MapParser.h"
#include "Lua/LuaParser.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/myMath.h"
#include "System/Util.h"

#if !defined(HEADLESS) && !defined(NO_SOUND)
#include "System/Sound/OpenAL/EFX.h"
#include "System/Sound/OpenAL/EFXPresets.h"
#endif

#include <cassert>
#include <cfloat>
#include <sstream>

using std::max;
using std::min;


// Before delete, the const is const_cast'ed away. There are
// no (other) situations where mapInfo may be modified, except
//   LuaUnsyncedCtrl may change water
//   LuaSyncedCtrl may change terrainTypes
const CMapInfo* mapInfo = NULL;


CMapInfo::CMapInfo(const std::string& mapInfoFile, const string& mapName)
{
	map.name = mapName;

	parser = new MapParser(mapInfoFile);
	if (!parser->IsValid()) {
		throw content_error("MapInfo: " + parser->GetErrorLog());
	}

	LuaParser resParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	LuaTable resTable;

	if (!resParser.Execute()) {
		LOG_L(L_ERROR, "%s", resParser.GetErrorLog().c_str());
	}

	resTable = resParser.GetRoot();
	resRoot = &resTable;

	ReadGlobal();
	ReadAtmosphere();
	ReadGui();
	ReadSplats();
	ReadGrass();
	ReadLight();
	ReadWater();
	ReadSMF();
	ReadSM3();
	ReadTerrainTypes();
	ReadPFSConstants();
	ReadSound();

	//FIXME save all data in an array, so we can destroy the lua context (to save mem)?
	//delete parser;
}

CMapInfo::~CMapInfo()
{
#if !defined(HEADLESS) && !defined(NO_SOUND)
	delete efxprops;
	efxprops = NULL;
#endif
	delete parser;
	parser = NULL;
}

std::string CMapInfo::GetStringValue(const std::string& key) const
{
	assert(parser->GetRoot().IsValid());
	return parser->GetRoot().GetString(key, "");
}

void CMapInfo::ReadGlobal()
{
	const LuaTable& topTable = parser->GetRoot();

	map.description  = topTable.GetString("description", map.name);
	map.author       = topTable.GetString("author", "");

	map.hardness      = topTable.GetFloat("maphardness", 100.0f);
	map.notDeformable = topTable.GetBool("notDeformable", false);

	map.gravity = topTable.GetFloat("gravity", 130.0f);
	map.gravity = max(0.001f, map.gravity);
	map.gravity = -map.gravity / (GAME_SPEED * GAME_SPEED);

	map.tidalStrength   = topTable.GetFloat("tidalStrength", 0.0f);
	map.maxMetal        = topTable.GetFloat("maxMetal", 0.02f);
	map.extractorRadius = topTable.GetFloat("extractorRadius", 500.0f);
	map.voidAlphaMin    = topTable.GetFloat("voidAlphaMin", 0.9f);
	map.voidWater       = topTable.GetBool("voidWater", false);
	map.voidGround      = topTable.GetBool("voidGround", false);

	// clamps
	if (-0.001f < map.hardness && map.hardness <= 0.0f)
		map.hardness = -0.001f;
	else if (0.0f <= map.hardness && map.hardness < 0.001f)
		map.hardness = 0.001f;
	map.tidalStrength   = max(0.000f, map.tidalStrength);
	map.maxMetal        = max(0.000f, map.maxMetal);
	map.extractorRadius = max(0.000f, map.extractorRadius);
}


void CMapInfo::ReadGui()
{
	// GUI
	gui.autoShowMetal = parser->GetRoot().GetBool("autoShowMetal", true);
}


void CMapInfo::ReadAtmosphere()
{
	// MAP\ATMOSPHERE
	const LuaTable& atmoTable = parser->GetRoot().SubTable("atmosphere");
	atmosphere_t& atmo = atmosphere;

	atmo.minWind = atmoTable.GetFloat("minWind", 5.0f);
	atmo.maxWind = atmoTable.GetFloat("maxWind", 25.0f);

	atmo.fogStart   = atmoTable.GetFloat("fogStart", 0.1f);
	atmo.fogEnd     = atmoTable.GetFloat("fogEnd",   1.0f);
	atmo.fogColor   = atmoTable.GetFloat3("fogColor", float3(0.7f, 0.7f, 0.8f));

	atmo.skyBox       = atmoTable.GetString("skyBox", "");
	atmo.skyColor     = atmoTable.GetFloat3("skyColor", float3(0.1f, 0.15f, 0.7f));
	atmo.skyDir       = atmoTable.GetFloat3("skyDir", -FwdVector);
	atmo.skyDir.ANormalize();
	atmo.sunColor     = atmoTable.GetFloat3("sunColor", float3(1.0f, 1.0f, 1.0f));
	atmo.cloudColor   = atmoTable.GetFloat3("cloudColor", float3(1.0f, 1.0f, 1.0f));
	atmo.fluidDensity = atmoTable.GetFloat("fluidDensity", 1.2f * 0.25f);
	atmo.cloudDensity = atmoTable.GetFloat("cloudDensity", 0.5f);

	// clamps
	atmo.cloudDensity = max(0.0f, atmo.cloudDensity);
	atmo.maxWind      = max(0.0f, atmo.maxWind);
	atmo.minWind      = max(0.0f, atmo.minWind);
	atmo.minWind      = min(atmo.maxWind, atmo.minWind);
}


void CMapInfo::ReadSplats()
{
	const LuaTable& splatsTable = parser->GetRoot().SubTable("splats");

	splats.texScales = splatsTable.GetFloat4("texScales", float4(0.02f, 0.02f, 0.02f, 0.02f));
	splats.texMults = splatsTable.GetFloat4("texMults", float4(1.0f, 1.0f, 1.0f, 1.0f));
}

void CMapInfo::ReadGrass()
{
	const LuaTable& grassTable = parser->GetRoot().SubTable("grass");
	const LuaTable& mapResTable = parser->GetRoot().SubTable("resources");

	grass.bladeWaveScale   = grassTable.GetFloat("bladeWaveScale", 1.0f);
	grass.bladeWidth       = grassTable.GetFloat("bladeWidth", 0.7f);
	grass.bladeHeight      = grassTable.GetFloat("bladeHeight", 4.5f);
	grass.bladeAngle       = grassTable.GetFloat("bladeAngle", 1.0f);
	grass.maxStrawsPerTurf = grassTable.GetInt("maxStrawsPerTurf", 150);
	grass.color            = grassTable.GetFloat3("bladeColor", float3(0.10f, 0.40f, 0.10f));

	grass.bladeTexName     = mapResTable.GetString("grassBladeTex", "");
	if (!grass.bladeTexName.empty()) { //FIXME only do when file doesn't exists under that path
		grass.bladeTexName = "maps/" + grass.bladeTexName;
	}
}

void CMapInfo::ReadLight()
{
	const LuaTable& lightTable = parser->GetRoot().SubTable("lighting");

	light.sunStartAngle = lightTable.GetFloat("sunStartAngle", 0.0f);
	light.sunOrbitTime = lightTable.GetFloat("sunOrbitTime", 1440.0f);
	light.sunDir = lightTable.GetFloat4("sunDir", float4(0.0f, 1.0f, 2.0f, FLT_MAX));

	if (light.sunDir.w == FLT_MAX) {
		// if four params are not specified for sundir, fallback to the old three param format
		light.sunDir = lightTable.GetFloat3("sunDir", float3(0.0f, 1.0f, 2.0f));
		light.sunDir.w = FLT_MAX;
	}

	light.sunDir.ANormalize();

	light.groundAmbientColor  = lightTable.GetFloat3("groundAmbientColor", float3(0.5f, 0.5f, 0.5f));
	light.groundSunColor      = lightTable.GetFloat3("groundDiffuseColor", float3(0.5f, 0.5f, 0.5f));
	light.groundSpecularColor = lightTable.GetFloat3("groundSpecularColor", float3(0.1f, 0.1f, 0.1f));
	light.groundShadowDensity = lightTable.GetFloat("groundShadowDensity", 0.8f);

	light.unitAmbientColor  = lightTable.GetFloat3("unitAmbientColor", float3(0.4f, 0.4f, 0.4f));
	light.unitSunColor      = lightTable.GetFloat3("unitDiffuseColor", float3(0.7f, 0.7f, 0.7f));
	light.unitSpecularColor = lightTable.GetFloat3("unitSpecularColor", light.unitSunColor);
	light.unitShadowDensity = lightTable.GetFloat("unitShadowDensity", 0.8f);

	light.specularExponent = lightTable.GetFloat("specularExponent", 100.0f);

	light.groundShadowDensity = Clamp(light.groundShadowDensity, 0.0f, 1.0f);
	light.unitShadowDensity = Clamp(light.unitShadowDensity, 0.0f, 1.0f);
}


void CMapInfo::ReadWater()
{
	const LuaTable& wt = parser->GetRoot().SubTable("water");

	water.fluidDensity = wt.GetFloat("fluidDensity", 960.0f * 0.25f);
	water.repeatX = wt.GetFloat("repeatX", 0.0f);
	water.repeatY = wt.GetFloat("repeatY", 0.0f);
	water.damage  = wt.GetFloat("damage",  0.0f) * ((float)UNIT_SLOWUPDATE_RATE / (float)GAME_SPEED);

	water.absorb    = wt.GetFloat3("absorb",    float3(0.0f, 0.0f, 0.0f));
	water.baseColor = wt.GetFloat3("baseColor", float3(0.0f, 0.0f, 0.0f));
	water.minColor  = wt.GetFloat3("minColor",  float3(0.0f, 0.0f, 0.0f));

	water.ambientFactor = wt.GetFloat("ambientFactor", 1.0f);
	water.diffuseFactor = wt.GetFloat("diffuseFactor", 1.0f);
	water.specularFactor= wt.GetFloat("specularFactor",1.0f);
	water.specularPower = wt.GetFloat("specularPower", 20.0f);

	water.planeColor    = wt.GetFloat3("planeColor", float3(0.0f, 0.4f, 0.0f));
	water.hasWaterPlane = wt.KeyExists("planeColor");

	water.surfaceColor  = wt.GetFloat3("surfaceColor", float3(0.75f, 0.8f, 0.85f));
	water.surfaceAlpha  = wt.GetFloat("surfaceAlpha",  0.55f);
	water.diffuseColor  = wt.GetFloat3("diffuseColor",  float3(1.0f, 1.0f, 1.0f));
	water.specularColor = wt.GetFloat3("specularColor", light.groundSunColor);

	water.fresnelMin   = wt.GetFloat("fresnelMin",   0.2f);
	water.fresnelMax   = wt.GetFloat("fresnelMax",   0.8f);
	water.fresnelPower = wt.GetFloat("fresnelPower", 4.0f);

	water.reflDistortion = wt.GetFloat("reflectionDistortion", 1.0f);

	water.blurBase     = wt.GetFloat("blurBase", 2.0f);
	water.blurExponent = wt.GetFloat("blurExponent", 1.5f);

	water.perlinStartFreq  = wt.GetFloat("perlinStartFreq",  8.0f);
	water.perlinLacunarity = wt.GetFloat("perlinLacunarity", 3.0f);
	water.perlinAmplitude  = wt.GetFloat("perlinAmplitude",  0.9f);
	water.windSpeed        = wt.GetFloat("windSpeed", 1.0f);

	water.texture       = wt.GetString("texture",       "");
	water.foamTexture   = wt.GetString("foamTexture",   "");
	water.normalTexture = wt.GetString("normalTexture", "");

	water.shoreWaves = wt.GetBool("shoreWaves", true);

	water.forceRendering = wt.GetBool("forceRendering", false);

	// use 'resources.lua' for missing fields  (our the engine defaults)
	const LuaTable& resGfxMaps = resRoot->SubTable("graphics").SubTable("maps");

	if (!water.texture.empty()) {
		water.texture = "maps/" + water.texture;
	} else {
		water.texture = "bitmaps/" + resGfxMaps.GetString("watertex", "ocean.jpg");
	}

	if (!water.foamTexture.empty()) {
		water.foamTexture = "maps/" + water.foamTexture;
	} else {
		water.foamTexture = "bitmaps/" + resGfxMaps.GetString("waterfoamtex", "foam.jpg");
	}

	if (!water.normalTexture.empty()) {
		water.normalTexture = "maps/" + water.normalTexture;
		water.numTiles    = std::min(16,std::max(1,wt.GetInt("numTiles",1)));
	} else {
		water.normalTexture = "bitmaps/" + resGfxMaps.GetString("waternormaltex", "waterbump.png");
		if (resGfxMaps.KeyExists("waternormaltex")) {
			water.numTiles = std::min(16,std::max(1,resGfxMaps.GetInt("numTiles",1)));
		}else{
			// default texture is a TileSet of 3x3
			// user-defined textures are expected to be 1x1 (no DynWaves possible)
			water.numTiles = 3;
		}
	}

	// water caustic textures
	LuaTable caustics = wt.SubTable("caustics");
	string causticPrefix = "maps/";
	if (!caustics.IsValid()) {
		caustics = resRoot->SubTable("graphics").SubTable("caustics");
		causticPrefix = "bitmaps/";
	}
	if (caustics.IsValid()) {
		for (int i = 1; true; i++) {
			const string texName = caustics.GetString(i, "");
			if (texName.empty()) {
				break;
			}
			water.causticTextures.push_back(causticPrefix + texName);
		}
	} else {
		// load the default 32 textures
		for (int i = 0; i < 32; i++) {
			char defTex[256];
			sprintf(defTex, "bitmaps/caustics/caustic%02i.jpg", i);
			water.causticTextures.push_back(defTex);
		}
	}
}


template<typename T> bool ParseSplatDetailNormalTexture(const LuaTable& table, const T key, std::vector<std::string>& texNames)
{
	if (!table.KeyExists(key))
		return false;

	const std::string& val = table.GetString(key, "");

	if (!val.empty()) {
		texNames.push_back("maps/" + val);
	} else {
		texNames.push_back("");
	}

	return true;
}

void CMapInfo::ReadSMF()
{
	// SMF specific settings
	const LuaTable& mapResTable = parser->GetRoot().SubTable("resources");
	const LuaTable& sdnTexTable = mapResTable.SubTable("splatDetailNormalTex");

	smf.detailTexName      = mapResTable.GetString("detailTex", "");
	smf.specularTexName    = mapResTable.GetString("specularTex", "");
	smf.splatDetailTexName = mapResTable.GetString("splatDetailTex", "");
	smf.splatDistrTexName  = mapResTable.GetString("splatDistrTex", "");
	smf.grassShadingTexName = mapResTable.GetString("grassShadingTex", "");
	smf.skyReflectModTexName  = mapResTable.GetString("skyReflectModTex", "");
	smf.blendNormalsTexName   = mapResTable.GetString("detailNormalTex", "");
	smf.lightEmissionTexName  = mapResTable.GetString("lightEmissionTex", "");
	smf.parallaxHeightTexName = mapResTable.GetString("parallaxHeightTex", "");

	if (!smf.detailTexName.empty()) {
		smf.detailTexName = "maps/" + smf.detailTexName;
	} else {
		const LuaTable& resGfxMaps = resRoot->SubTable("graphics").SubTable("maps");
		smf.detailTexName = resGfxMaps.GetString("detailtex", "detailtex2.bmp");
		smf.detailTexName = "bitmaps/" + smf.detailTexName;
	}

	const std::vector<std::string*> texNames = {
		&smf.specularTexName,
		&smf.splatDetailTexName,
		&smf.splatDistrTexName,
		&smf.grassShadingTexName,
		&smf.skyReflectModTexName,
		&smf.blendNormalsTexName,
		&smf.lightEmissionTexName,
		&smf.parallaxHeightTexName,
	};

	for (size_t i = 0; i < texNames.size(); i++) {
		if (!texNames[i]->empty()) {
			*texNames[i] = "maps/" + *texNames[i];
		}
	}

	if (sdnTexTable.IsValid()) {
		smf.splatDetailNormalDiffuseAlpha = sdnTexTable.GetBool("alpha", false);

		for (int i = 0; true; i++) {
			if (!ParseSplatDetailNormalTexture(sdnTexTable, i + 1, smf.splatDetailNormalTexNames)) {
				break;
			}
		}
	} else {
		smf.splatDetailNormalDiffuseAlpha = mapResTable.GetBool("splatDetailNormalDiffuseAlpha", false);

		for (int i = 0; true; i++) {
			if (!ParseSplatDetailNormalTexture(mapResTable, "splatDetailNormalTex" + IntToString(i + 1), smf.splatDetailNormalTexNames)) {
				break;
			}
		}
	}


	// overrides for compiled parameters
	const LuaTable& smfTable = parser->GetRoot().SubTable("smf");

	smf.minHeightOverride = smfTable.KeyExists("minHeight");
	smf.maxHeightOverride = smfTable.KeyExists("maxHeight");
	smf.minHeight         = smfTable.GetFloat("minHeight", 0.0f);
	smf.maxHeight         = smfTable.GetFloat("maxHeight", 0.0f);

	smf.minimapTexName  = smfTable.GetString("minimapTex", "");
	smf.metalmapTexName = smfTable.GetString("metalmapTex", "");
	smf.typemapTexName  = smfTable.GetString("typemapTex", "");
	smf.grassmapTexName = smfTable.GetString("grassmapTex", "");

	if (!smf.minimapTexName.empty() ) { smf.minimapTexName  = "maps/" + smf.minimapTexName; }
	if (!smf.metalmapTexName.empty()) { smf.metalmapTexName = "maps/" + smf.metalmapTexName; }
	if (!smf.typemapTexName.empty() ) { smf.typemapTexName  = "maps/" + smf.typemapTexName; }
	if (!smf.grassmapTexName.empty()) { smf.grassmapTexName = "maps/" + smf.grassmapTexName; }

	std::stringstream ss;

	for (int i = 0; /* no test */; i++) {
		ss.str("");
		ss << "smtFileName" << i;

		if (smfTable.KeyExists(ss.str())) {
			smf.smtFileNames.push_back(smfTable.GetString(ss.str(), ".smt"));
		} else {
			break;
		}
	}
}


void CMapInfo::ReadSM3()
{
	// SM3 specific settings
	sm3.minimap = parser->GetRoot().GetString("minimap", "");
}


void CMapInfo::ReadTerrainTypes()
{
	const LuaTable& terrTypeTable = parser->GetRoot().SubTable("terrainTypes");

	for (int tt = 0; tt < NUM_TERRAIN_TYPES; tt++) {
		TerrainType& terrType = terrainTypes[tt];
		const LuaTable& terrain = terrTypeTable.SubTable(tt);
		const LuaTable& moveTable = terrain.SubTable("moveSpeeds");

		terrType.name          = terrain.GetString("name", "Default");
		terrType.hardness      = terrain.GetFloat("hardness",   1.0f);
		terrType.receiveTracks = terrain.GetBool("receiveTracks", true);

		terrType.tankSpeed  = moveTable.GetFloat("tank",  1.0f);
		terrType.kbotSpeed  = moveTable.GetFloat("kbot",  1.0f);
		terrType.hoverSpeed = moveTable.GetFloat("hover", 1.0f);
		terrType.shipSpeed  = moveTable.GetFloat("ship",  1.0f);

		// clamps
		terrType.hardness   = max(0.001f, terrType.hardness);
		terrType.tankSpeed  = max(0.000f, terrType.tankSpeed);
		terrType.kbotSpeed  = max(0.000f, terrType.kbotSpeed);
		terrType.hoverSpeed = max(0.000f, terrType.hoverSpeed);
		terrType.shipSpeed  = max(0.000f, terrType.shipSpeed);
	}
}

void CMapInfo::ReadPFSConstants()
{
	const LuaTable& pfsTable = (parser->GetRoot()).SubTable("pfs");
//	const LuaTable& legacyTable = pfsTable.SubTable("legacyConstants");
	const LuaTable& qtpfsTable = pfsTable.SubTable("qtpfsConstants");

//	pfs_t::legacy_constants_t& legacyConsts = pfs.legacy_constants;
	pfs_t::qtpfs_constants_t& qtpfsConsts = pfs.qtpfs_constants;

	qtpfsConsts.layersPerUpdate = qtpfsTable.GetInt("layersPerUpdate",  5);
	qtpfsConsts.maxTeamSearches = qtpfsTable.GetInt("maxTeamSearches", 25);
	qtpfsConsts.minNodeSizeX    = qtpfsTable.GetInt("minNodeSizeX",     8);
	qtpfsConsts.minNodeSizeZ    = qtpfsTable.GetInt("minNodeSizeZ",     8);
	qtpfsConsts.maxNodeDepth    = qtpfsTable.GetInt("maxNodeDepth",    16);
	qtpfsConsts.numSpeedModBins = qtpfsTable.GetInt("numSpeedModBins", 10);
	qtpfsConsts.minSpeedModVal  = std::max(                      0.0f, qtpfsTable.GetFloat("minSpeedModVal", 0.0f));
	qtpfsConsts.maxSpeedModVal  = std::max(qtpfsConsts.minSpeedModVal, qtpfsTable.GetFloat("maxSpeedModVal", 2.0f));
}

void CMapInfo::ReadSound()
{
#if !defined(HEADLESS) && !defined(NO_SOUND)
	const LuaTable& soundTable = parser->GetRoot().SubTable("sound");

	efxprops = new EAXSfxProps();

	const std::string presetname = soundTable.GetString("preset", "default");
	std::map<std::string, EAXSfxProps>::const_iterator et = eaxPresets.find(presetname);
	if (et != eaxPresets.end()) {
		*efxprops = et->second;
	}

	std::map<std::string, ALuint>::const_iterator it;

	const LuaTable& filterTable = soundTable.SubTable("passfilter");
	for (it = nameToALFilterParam.begin(); it != nameToALFilterParam.end(); ++it) {
		const std::string& name = it->first;
		const int luaType = filterTable.GetType(name);

		if (luaType == LuaTable::NIL)
			continue;
		
		const ALuint param = it->second;
		const unsigned& type = alParamType[param];
		switch (type) {
			case EFXParamTypes::FLOAT:
				if (luaType == LuaTable::NUMBER)
					efxprops->filter_properties_f[param] = filterTable.GetFloat(name, 0.f);
				break;
		}
	}

	soundTable.SubTable("reverb");
	for (it = nameToALParam.begin(); it != nameToALParam.end(); ++it) {
		const std::string& name = it->first;
		const int luaType = filterTable.GetType(name);

		if (luaType == LuaTable::NIL)
			continue;

		const ALuint param = it->second;
		const unsigned& type = alParamType[param];
		switch (type) {
			case EFXParamTypes::VECTOR:
				if (luaType == LuaTable::TABLE)
					efxprops->properties_v[param] = filterTable.GetFloat3(name, ZeroVector);
				break;
			case EFXParamTypes::FLOAT:
				if (luaType == LuaTable::NUMBER)
					efxprops->properties_f[param] = filterTable.GetFloat(name, 0.f);
				break;
			case EFXParamTypes::BOOL:
				if (luaType == LuaTable::BOOLEAN)
					efxprops->properties_i[param] = filterTable.GetBool(name, false);
				break;
		}
	}
#endif
}
