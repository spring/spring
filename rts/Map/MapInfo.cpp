/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "MapInfo.h"

#include "Sim/Misc/GlobalConstants.h"
#include "Rendering/GlobalRendering.h"

#include "Lua/LuaParser.h"
#include "System/Log/ILog.h"
#include "System/Exceptions.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/FileSystem/FileHandler.h"

#if !defined(HEADLESS) && !defined(NO_SOUND)
#include "System/Sound/OpenAL/EFX.h"
#include "System/Sound/OpenAL/EFXPresets.h"
#endif

#include <array>
#include <cassert>


// Before delete, the const is const_cast'ed away. There are
// no (other) situations where mapInfo may be modified, except
//   LuaUnsyncedCtrl may change water
//   LuaSyncedCtrl may change terrainTypes
const CMapInfo* mapInfo = nullptr;


static void FIND_MAP_TEXTURE(std::string* filePath, const std::string& defaultDir = "maps/")
{
	if (filePath->empty())
		return;
	if (CFileHandler::FileExists(*filePath, SPRING_VFS_ZIP)) // no RawFS, cause it's also used for synced textures (typemap, metalmap, ...)
		return;

	*filePath = defaultDir + *filePath;
}



CMapInfo::CMapInfo(const std::string& mapFileName, const string& mapHumanName): mapInfoParser(mapFileName)
{
	map.name = mapHumanName;

	if (!mapInfoParser.IsValid())
		throw content_error("[MapInfo] info-parser for map \"" + map.name + "\" (\"" + mapFileName + "\") invalid: \"" + mapInfoParser.GetErrorLog() + "\"");

	LuaParser resParser("gamedata/resources.lua", SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	LuaTable resTable;

	if (!resParser.Execute())
		LOG_L(L_ERROR, "[%s] error executing resource-parser: \"%s\"", __func__, resParser.GetErrorLog().c_str());

	resTable = resParser.GetRoot();
	resTableRoot = &resTable;

	ReadGlobal();
	ReadAtmosphere();
	ReadGui();
	ReadSplats();
	ReadGrass();
	ReadLight();
	ReadWater();
	ReadSMF();
	ReadTerrainTypes();
	ReadPFSConstants();
	ReadSound();

	//FIXME save all mapParser data in an array, so we can destroy the lua context (to save mem)?
}

CMapInfo::~CMapInfo()
{
#if !defined(HEADLESS) && !defined(NO_SOUND)
	delete efxprops;
	efxprops = nullptr;
#endif
}


void CMapInfo::ReadGlobal()
{
	const LuaTable& topTable = mapInfoParser.GetRoot();

	map.description  = topTable.GetString("description", map.name);
	map.author       = topTable.GetString("author", "");

	map.hardness      = topTable.GetFloat("maphardness", 100.0f);
	map.notDeformable = topTable.GetBool("notDeformable", false);

	map.gravity = topTable.GetFloat("gravity", 130.0f);
	map.gravity = std::max(0.001f, map.gravity);
	map.gravity = -map.gravity / (GAME_SPEED * GAME_SPEED);

	map.tidalStrength   = topTable.GetFloat("tidalStrength", 0.0f);
	map.maxMetal        = topTable.GetFloat("maxMetal", 0.02f);
	map.extractorRadius = topTable.GetFloat("extractorRadius", 500.0f);
	map.voidAlphaMin    = topTable.GetFloat("voidAlphaMin", 0.9f);
	map.voidWater       = topTable.GetBool("voidWater", false);
	map.voidGround      = topTable.GetBool("voidGround", false);

	// clamps; global hardness can be either positive or negative but never zero
	map.hardness        = std::max(0.001f, std::abs(map.hardness)) * std::copysign(1.0f, map.hardness);
	map.tidalStrength   = std::max(0.000f, map.tidalStrength);
	map.maxMetal        = std::max(0.000f, map.maxMetal);
	map.extractorRadius = std::max(0.000f, map.extractorRadius);
}


void CMapInfo::ReadGui()
{
	// GUI
	gui.autoShowMetal = mapInfoParser.GetRoot().GetBool("autoShowMetal", true);
}


void CMapInfo::ReadAtmosphere()
{
	// MAP\ATMOSPHERE
	const LuaTable& atmoTable = mapInfoParser.GetRoot().SubTable("atmosphere");
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
	atmo.cloudDensity = std::max(0.0f, atmo.cloudDensity);
	atmo.maxWind      = std::max(0.0f, atmo.maxWind);
	atmo.minWind      = std::max(0.0f, atmo.minWind);
	atmo.minWind      = std::min(atmo.maxWind, atmo.minWind);
}


void CMapInfo::ReadSplats()
{
	const LuaTable& splatsTable = mapInfoParser.GetRoot().SubTable("splats");

	splats.texScales = splatsTable.GetFloat4("texScales", float4(0.02f, 0.02f, 0.02f, 0.02f));
	splats.texMults  = splatsTable.GetFloat4("texMults",  float4(1.0f, 1.0f, 1.0f, 1.0f));
}

void CMapInfo::ReadGrass()
{
	const LuaTable& grassTable = mapInfoParser.GetRoot().SubTable("grass");
	const LuaTable& mapResTable = mapInfoParser.GetRoot().SubTable("resources");

	grass.bladeWaveScale   = grassTable.GetFloat("bladeWaveScale", 1.0f);
	grass.bladeWidth       = grassTable.GetFloat("bladeWidth", 0.7f);
	grass.bladeHeight      = grassTable.GetFloat("bladeHeight", 4.5f);
	grass.bladeAngle       = grassTable.GetFloat("bladeAngle", 1.0f);
	grass.maxStrawsPerTurf = grassTable.GetInt("maxStrawsPerTurf", 150);
	grass.color            = grassTable.GetFloat3("bladeColor", float3(0.10f, 0.40f, 0.10f));

	grass.bladeTexName     = mapResTable.GetString("grassBladeTex", "");
	FIND_MAP_TEXTURE(&grass.bladeTexName);
}

void CMapInfo::ReadLight()
{
	const LuaTable& lightTable = mapInfoParser.GetRoot().SubTable("lighting");

	// read the float4 direction first; keep it if the float3 value does not exist
	light.sunDir =  lightTable.GetFloat4("sunDir", float4(0.0f, 1.0f, 2.0f, 1.0f));
	light.sunDir = {lightTable.GetFloat3("sunDir", light.sunDir), light.sunDir.w};
	light.sunDir.ANormalize();

	light.groundAmbientColor  = lightTable.GetFloat3("groundAmbientColor", float3(0.5f, 0.5f, 0.5f));
	light.groundDiffuseColor  = lightTable.GetFloat3("groundDiffuseColor", float3(0.5f, 0.5f, 0.5f));
	light.groundSpecularColor = lightTable.GetFloat3("groundSpecularColor", float3(0.1f, 0.1f, 0.1f));
	light.groundShadowDensity = lightTable.GetFloat("groundShadowDensity", 0.8f);

	light.modelAmbientColor  = lightTable.GetFloat3("unitAmbientColor", float3(0.4f, 0.4f, 0.4f));
	light.modelDiffuseColor  = lightTable.GetFloat3("unitDiffuseColor", float3(0.7f, 0.7f, 0.7f));
	light.modelSpecularColor = lightTable.GetFloat3("unitSpecularColor", light.modelDiffuseColor);
	light.modelShadowDensity = lightTable.GetFloat("unitShadowDensity", 0.8f);

	light.specularExponent = lightTable.GetFloat("specularExponent", 100.0f);

	light.groundShadowDensity = Clamp(light.groundShadowDensity, 0.0f, 1.0f);
	light.modelShadowDensity   = Clamp(light.modelShadowDensity,   0.0f, 1.0f);
}


void CMapInfo::ReadWater()
{
	const LuaTable& wt = mapInfoParser.GetRoot().SubTable("water");

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
	water.specularColor = wt.GetFloat3("specularColor", light.groundDiffuseColor);

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
	const LuaTable& resGfxMaps = resTableRoot->SubTable("graphics").SubTable("maps");

	FIND_MAP_TEXTURE(&water.texture);
	if (water.texture.empty()) {
		water.texture = resGfxMaps.GetString("watertex", "ocean.jpg");
		FIND_MAP_TEXTURE(&water.texture, "bitmaps/");
	}

	FIND_MAP_TEXTURE(&water.foamTexture);
	if (water.foamTexture.empty()) {
		water.foamTexture = resGfxMaps.GetString("waterfoamtex", "foam.jpg");
		FIND_MAP_TEXTURE(&water.foamTexture, "bitmaps/");
	}

	FIND_MAP_TEXTURE(&water.normalTexture);
	water.numTiles = Clamp(wt.GetInt("numTiles", 1), 1, 16);
	if (water.normalTexture.empty()) {
		water.normalTexture = resGfxMaps.GetString("waternormaltex", "waterbump.png");
		FIND_MAP_TEXTURE(&water.normalTexture, "bitmaps/");

		// default texture is a TileSet of 3x3
		// user-defined textures are expected to be 1x1 (no DynWaves possible)
		water.numTiles = 3;

		if (resGfxMaps.KeyExists("waternormaltex")) {
			water.numTiles = Clamp(resGfxMaps.GetInt("numTiles", 1), 1, 16);
		}
	}

	// water caustic textures
	LuaTable caustics = wt.SubTable("caustics");
	std::string causticPrefix = "maps/";
	if (!caustics.IsValid()) {
		caustics = resTableRoot->SubTable("graphics").SubTable("caustics");
		causticPrefix = "bitmaps/";
	}
	if (caustics.IsValid()) {
		for (int i = 1; true; i++) {
			std::string texName = caustics.GetString(i, "");
			if (texName.empty())
				break;

			FIND_MAP_TEXTURE(&texName, causticPrefix);
			water.causticTextures.push_back(texName);
		}
	} else {
		// load the default 32 textures
		for (int i = 0; i < 32; i++) {
			water.causticTextures.push_back(IntToString(i, "bitmaps/caustics/caustic%02i.jpg"));
		}
	}
}


template<typename T>
static bool ParseSplatDetailNormalTexture(const LuaTable& table, const T key, std::vector<std::string>& texNames)
{
	if (!table.KeyExists(key))
		return false;

	std::string val = table.GetString(key, "");
	FIND_MAP_TEXTURE(&val);
	texNames.push_back(val);
	return true;
}


void CMapInfo::ReadSMF()
{
	// SMF specific settings
	const LuaTable& mapResTable = mapInfoParser.GetRoot().SubTable("resources");
	const LuaTable& sdnTexTable = mapResTable.SubTable("splatDetailNormalTex");

	const std::array<std::pair<std::string*, std::string>, 9> texNames = {{
		{&smf.detailTexName,         "detailTex"},
		{&smf.specularTexName,       "specularTex"},
		{&smf.splatDetailTexName,    "splatDetailTex"},
		{&smf.splatDistrTexName,     "splatDistrTex"},
		{&smf.grassShadingTexName,   "grassShadingTex"},
		{&smf.skyReflectModTexName,  "skyReflectModTex"},
		{&smf.blendNormalsTexName,   "detailNormalTex"},
		{&smf.lightEmissionTexName,  "lightEmissionTex"},
		{&smf.parallaxHeightTexName, "parallaxHeightTex"},
	}};

	for (auto& pair: texNames) {
		*pair.first = mapResTable.GetString(pair.second, "");
		FIND_MAP_TEXTURE(pair.first);
	}

	if (smf.detailTexName.empty()) {
		const LuaTable& resGfxMaps = resTableRoot->SubTable("graphics").SubTable("maps");
		smf.detailTexName = resGfxMaps.GetString("detailtex", "detailtex2.bmp");
		FIND_MAP_TEXTURE(&smf.detailTexName, "bitmaps/");
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
	const LuaTable& smfTable = mapInfoParser.GetRoot().SubTable("smf");

	smf.minHeightOverride = smfTable.KeyExists("minHeight");
	smf.maxHeightOverride = smfTable.KeyExists("maxHeight");
	smf.minHeight         = smfTable.GetFloat("minHeight", 0.0f);
	smf.maxHeight         = smfTable.GetFloat("maxHeight", 0.0f);

	smf.minimapTexName  = smfTable.GetString("minimapTex",  "");
	smf.metalmapTexName = smfTable.GetString("metalmapTex", "");
	smf.typemapTexName  = smfTable.GetString("typemapTex",  "");
	smf.grassmapTexName = smfTable.GetString("grassmapTex", "");

	FIND_MAP_TEXTURE(&smf.minimapTexName);
	FIND_MAP_TEXTURE(&smf.metalmapTexName);
	FIND_MAP_TEXTURE(&smf.typemapTexName);
	FIND_MAP_TEXTURE(&smf.grassmapTexName);

	for (int i = 0; /* no test */; i++) {
		const std::string key = IntToString(i, "smtFileName%i");

		if (!smfTable.KeyExists(key))
			break;

		smf.smtFileNames.push_back(smfTable.GetString(key, ".smt"));
	}
}


void CMapInfo::ReadTerrainTypes()
{
	const LuaTable& terrTypeTable = mapInfoParser.GetRoot().SubTable("terrainTypes");

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
		terrType.hardness   = std::max(0.001f, terrType.hardness);
		terrType.tankSpeed  = std::max(0.000f, terrType.tankSpeed);
		terrType.kbotSpeed  = std::max(0.000f, terrType.kbotSpeed);
		terrType.hoverSpeed = std::max(0.000f, terrType.hoverSpeed);
		terrType.shipSpeed  = std::max(0.000f, terrType.shipSpeed);
	}
}

void CMapInfo::ReadPFSConstants()
{
	const LuaTable& pfsTable = (mapInfoParser.GetRoot()).SubTable("pfs");
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
	const LuaTable& soundTable = mapInfoParser.GetRoot().SubTable("sound");

	efxprops = new EAXSfxProps();

	const auto presetIt = eaxPresets.find(soundTable.GetString("preset", "default"));

	if (presetIt != eaxPresets.end())
		*efxprops = presetIt->second;

	const LuaTable& filterTable = soundTable.SubTable("passfilter");

	for (const auto& item: nameToALFilterParam) {
		const std::string& name = item.first;
		const int luaType = filterTable.GetType(name);

		if (luaType == LuaTable::NIL)
			continue;

		const ALuint param = item.second;
		const unsigned type = alParamType[param];

		switch (type) {
			case EFXParamTypes::FLOAT:
				if (luaType == LuaTable::NUMBER)
					efxprops->filter_props_f[param] = filterTable.GetFloat(name, 0.0f);
				break;
		}
	}

	soundTable.SubTable("reverb");

	for (const auto& item: nameToALFilterParam) {
		const std::string& name = item.first;
		const int luaType = filterTable.GetType(name);

		if (luaType == LuaTable::NIL)
			continue;

		const ALuint param = item.second;
		const unsigned type = alParamType[param];

		switch (type) {
			case EFXParamTypes::VECTOR:
				if (luaType == LuaTable::TABLE)
					efxprops->reverb_props_v[param] = filterTable.GetFloat3(name, ZeroVector);
				break;
			case EFXParamTypes::FLOAT:
				if (luaType == LuaTable::NUMBER)
					efxprops->reverb_props_f[param] = filterTable.GetFloat(name, 0.0f);
				break;
			case EFXParamTypes::BOOL:
				if (luaType == LuaTable::BOOLEAN)
					efxprops->reverb_props_i[param] = filterTable.GetBool(name, false);
				break;
		}
	}
#endif
}
