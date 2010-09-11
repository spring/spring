/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "MapInfo.h"

#include <assert.h>

#include "Sim/Misc/GlobalConstants.h"
#include "MapParser.h"
#include "Lua/LuaParser.h"
#include "LogOutput.h"
#include "Exceptions.h"


using namespace std;


// Before delete, the const is const_cast'ed away. There are
// no (other) situations where mapInfo may be modified, except
//   LuaUnsyncedCtrl may change water
//   LuaSyncedCtrl may change terrainTypes
const CMapInfo* mapInfo;


CMapInfo::CMapInfo(const std::string& _mapInfoFile, const string& mapName) : mapInfoFile(_mapInfoFile)
{
	map.name = mapName;

	parser = new MapParser(mapInfoFile);
	if (!parser->IsValid()) {
		throw content_error("MapInfo: " + parser->GetErrorLog());
	}

	resRoot = NULL;
}

void CMapInfo::Load()
{
	LuaParser resParser("gamedata/resources.lua",
	                    SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!resParser.Execute()) {
		logOutput.Print(resParser.GetErrorLog());
	}
	LuaTable resTbl = resParser.GetRoot();
	resRoot = &resTbl;

	ReadGlobal();
	ReadAtmosphere();
	ReadGui();
	ReadSplats();
	ReadGrass();
	ReadLight();
	ReadWater();
	ReadSmf();
	ReadSm3();
	ReadTerrainTypes();
}

CMapInfo::~CMapInfo()
{
	delete parser;
}

std::string CMapInfo::GetStringValue(const std::string& key) const
{
	assert(parser->GetRoot().IsValid());
	return parser->GetRoot().GetString(key, "");
}

void CMapInfo::ReadGlobal()
{
	const LuaTable topTable = parser->GetRoot();

	map.humanName    = topTable.GetString("description", map.name);
	map.author       = topTable.GetString("author", "");

	map.hardness      = std::max(0.001f, topTable.GetFloat("maphardness", 100.0f));
	map.notDeformable = topTable.GetBool("notDeformable", false);

	map.gravity = topTable.GetFloat("gravity", 130.0f);
	map.gravity = max(0.001f, map.gravity);
	map.gravity = -map.gravity / (GAME_SPEED * GAME_SPEED);

	map.tidalStrength   = topTable.GetFloat("tidalStrength", 0.0f);
	map.maxMetal        = topTable.GetFloat("maxMetal", 0.02f);
	map.extractorRadius = topTable.GetFloat("extractorRadius", 500.0f);

	map.voidWater = topTable.GetBool("voidWater", false);

	// clamps
	map.hardness        = max(0.0f, map.hardness);
	map.tidalStrength   = max(0.0f, map.tidalStrength);
	map.maxMetal        = max(0.0f, map.maxMetal);
	map.extractorRadius = max(0.0f, map.extractorRadius);
}


void CMapInfo::ReadGui()
{
	// GUI
	gui.autoShowMetal = parser->GetRoot().GetBool("autoShowMetal", true);
}


void CMapInfo::ReadAtmosphere()
{
	// MAP\ATMOSPHERE
	const LuaTable atmoTable = parser->GetRoot().SubTable("atmosphere");
	atmosphere_t& atmo = atmosphere;
	atmo.cloudDensity = atmoTable.GetFloat("cloudDensity", 0.5f);
	atmo.minWind      = atmoTable.GetFloat("minWind", 5.0f);
	atmo.maxWind      = atmoTable.GetFloat("maxWind", 25.0f);
	atmo.fogStart     = atmoTable.GetFloat("fogStart", 0.1f);
	atmo.fogColor   = atmoTable.GetFloat3("fogColor", float3(0.7f, 0.7f, 0.8f));
	atmo.skyColor   = atmoTable.GetFloat3("skyColor", float3(0.1f, 0.15f, 0.7f));
	atmo.sunColor   = atmoTable.GetFloat3("sunColor", float3(1.0f, 1.0f, 1.0f));
	atmo.cloudColor = atmoTable.GetFloat3("cloudColor", float3(1.0f, 1.0f, 1.0f));
	atmo.skyBox = atmoTable.GetString("skyBox", "");

	// clamps
	atmo.cloudDensity = max(0.0f, atmo.cloudDensity);
	atmo.maxWind      = max(0.0f, atmo.maxWind);
	atmo.minWind      = max(0.0f, atmo.minWind);
	atmo.minWind      = min(atmo.maxWind, atmo.minWind);
}


void CMapInfo::ReadSplats()
{
	const LuaTable splatsTable = parser->GetRoot().SubTable("splats");

	splats.texScales = splatsTable.GetFloat4("texScales", float4(0.02f, 0.02f, 0.02f, 0.02f));
	splats.texMults = splatsTable.GetFloat4("texMults", float4(1.0f, 1.0f, 1.0f, 1.0f));
}

void CMapInfo::ReadGrass()
{
	const LuaTable grassTable = parser->GetRoot().SubTable("grass");

	grass.bladeWaveScale = grassTable.GetFloat("bladeWaveScale", 1.0f);
	grass.bladeWidth     = grassTable.GetFloat("bladeWidth", 0.32f);
	grass.bladeHeight    = grassTable.GetFloat("bladeHeight", 4.0f);
	grass.bladeAngle     = grassTable.GetFloat("bladeAngle", 1.57f);
}

void CMapInfo::ReadLight()
{
	const LuaTable lightTable = parser->GetRoot().SubTable("lighting");

	light.sunDir = lightTable.GetFloat3("sunDir", float3(0.0f, 1.0f, 2.0f));
	light.sunDir.ANormalize();

	light.groundAmbientColor  = lightTable.GetFloat3("groundAmbientColor",
	                                                float3(0.5f, 0.5f, 0.5f));
	light.groundSunColor      = lightTable.GetFloat3("groundDiffuseColor",
	                                                float3(0.5f, 0.5f, 0.5f));
	light.groundSpecularColor = lightTable.GetFloat3("groundSpecularColor",
	                                                float3(0.1f, 0.1f, 0.1f));
	light.groundShadowDensity = lightTable.GetFloat("groundShadowDensity", 0.8f);

	light.unitAmbientColor  = lightTable.GetFloat3("unitAmbientColor",
	                                                float3(0.4f, 0.4f, 0.4f));
	light.unitSunColor      = lightTable.GetFloat3("unitDiffuseColor",
	                                                float3(0.7f, 0.7f, 0.7f));
	light.unitSpecularColor  = lightTable.GetFloat3("unitSpecularColor",
	                                               light.unitSunColor);
	light.unitShadowDensity = lightTable.GetFloat("unitShadowDensity", 0.8f);
}


void CMapInfo::ReadWater()
{
	const LuaTable wt = parser->GetRoot().SubTable("water");

	water.repeatX = wt.GetFloat("repeatX", 0.0f);
	water.repeatY = wt.GetFloat("repeatY", 0.0f);
	water.damage  = wt.GetFloat("damage",  0.0f) * (16.0f / 30.0f);

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
	const LuaTable resGfxMaps = resRoot->SubTable("graphics").SubTable("maps");

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


void CMapInfo::ReadSmf()
{
	// SMF specific settings
	const LuaTable mapResTable = parser->GetRoot().SubTable("resources");

	smf.detailTexName      = mapResTable.GetString("detailTex", "");
	smf.specularTexName    = mapResTable.GetString("specularTex", "");
	smf.splatDetailTexName = mapResTable.GetString("splatDetailTex", "");
	smf.splatDistrTexName  = mapResTable.GetString("splatDistrTex", "");

	smf.grassBladeTexName = mapResTable.GetString("grassBladeTex", "");
	smf.grassShadingTexName = mapResTable.GetString("grassShadingTex", "");

	smf.skyReflectModTexName = mapResTable.GetString("skyReflectModTex", "");

	if (!smf.detailTexName.empty()) {
		smf.detailTexName = "maps/" + smf.detailTexName;
	} else {
		const LuaTable resGfxMaps = resRoot->SubTable("graphics").SubTable("maps");
		smf.detailTexName = resGfxMaps.GetString("detailtex", "detailtex2.bmp");
		smf.detailTexName = "bitmaps/" + smf.detailTexName;
	}

	if (!smf.specularTexName.empty()) { smf.specularTexName = "maps/" + smf.specularTexName; }
	if (!smf.splatDetailTexName.empty()) { smf.splatDetailTexName = "maps/" + smf.splatDetailTexName; }
	if (!smf.splatDistrTexName.empty()) { smf.splatDistrTexName = "maps/" + smf.splatDistrTexName; }
	if (!smf.grassBladeTexName.empty()) { smf.grassBladeTexName = "maps/" + smf.grassBladeTexName; }
	if (!smf.grassShadingTexName.empty()) { smf.grassShadingTexName = "maps/" + smf.grassShadingTexName; }
	if (!smf.skyReflectModTexName.empty()) { smf.skyReflectModTexName = "maps/" + smf.skyReflectModTexName; }

	// height overrides
	const LuaTable smfTable = parser->GetRoot().SubTable("smf");

	smf.minHeightOverride = smfTable.KeyExists("minHeight");
	smf.maxHeightOverride = smfTable.KeyExists("maxHeight");
	smf.minHeight = smfTable.GetFloat("minHeight", 0.0f);
	smf.maxHeight = smfTable.GetFloat("maxHeight", 0.0f);


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


void CMapInfo::ReadSm3()
{
	// SM3 specific settings
	sm3.minimap = parser->GetRoot().GetString("minimap", "");
}


void CMapInfo::ReadTerrainTypes()
{
	const LuaTable terrTypeTable =
		parser->GetRoot().SubTable("terrainTypes");

	for (int tt = 0; tt < NUM_TERRAIN_TYPES; tt++) {
		TerrainType& terrType = terrainTypes[tt];
		const LuaTable terrain = terrTypeTable.SubTable(tt);
		terrType.name          = terrain.GetString("name", "Default");
		terrType.hardness      = terrain.GetFloat("hardness",   1.0f);
		terrType.receiveTracks = terrain.GetBool("receiveTracks", true);
		const LuaTable moveTable = terrain.SubTable("moveSpeeds");
		terrType.tankSpeed  = moveTable.GetFloat("tank",  1.0f);
		terrType.kbotSpeed  = moveTable.GetFloat("kbot",  1.0f);
		terrType.hoverSpeed = moveTable.GetFloat("hover", 1.0f);
		terrType.shipSpeed  = moveTable.GetFloat("ship",  1.0f);

		// clamps
		terrType.hardness   = max(0.0f, terrType.hardness);
		terrType.tankSpeed  = max(0.0f, terrType.tankSpeed);
		terrType.kbotSpeed  = max(0.0f, terrType.kbotSpeed);
		terrType.hoverSpeed = max(0.0f, terrType.hoverSpeed);
		terrType.shipSpeed  = max(0.0f, terrType.shipSpeed);
	}
}
