#include "MapInfo.h"

#include "TdfParser.h"


// before delete, the const is const_cast'ed away.
// there are no (other) situations where mapInfo may be modified
const CMapInfo* mapInfo;


CMapInfo::CMapInfo(const std::string& mapname)
{
	map.name = mapname;
	mapDefParser = new TdfParser(GetTDFName(mapname));
	resources = new TdfParser("gamedata/resources.tdf");

	ReadGlobal();
	ReadAtmosphere();
	ReadGui();
	ReadLight();
	ReadWater();
	ReadSmf();
	ReadSm3();
	ReadTerrainTypes();

	delete resources;
	resources = NULL;
}


CMapInfo::~CMapInfo()
{
	delete mapDefParser;
	mapDefParser = NULL;
}


/** @brief Opens the TDF file from the given map in parser.
	FIXME: This is mostly a hack to supply CGameSetup with start positions,
	when no CMapInfo object has yet been created.
 */
void CMapInfo::OpenTDF(const std::string& mapname, TdfParser& parser)
{
	parser.LoadFile(GetTDFName(mapname));
}


/** @brief Get the name of the TDF file with map settings.
	@return "maps/%.smd" for "%.smf" and "maps/%.sm3" for "%.sm3"
 */
std::string CMapInfo::GetTDFName(const std::string& mapname)
{
	if (mapname.length() < 3)
		throw std::runtime_error("CMapInfo::GetTDFName(): mapname '" + mapname + "' too short");

	std::string extension = mapname.substr(mapname.length() - 3);
	if (extension == "smf")
		return std::string("maps/") + mapname.substr(0, mapname.find_last_of('.')) + ".smd";
	else if(extension == "sm3")
		return std::string("maps/") + mapname;
	else
		throw std::runtime_error("CMapInfo::GetTDFName(): Unknown extension: " + extension);
}


void CMapInfo::ReadGlobal()
{
	map.humanName = mapDefParser->SGetValueDef(map.name, "MAP\\Description");
	map.wantedScript = mapDefParser->SGetValueDef(map.wantedScript, "MAP\\Description");

	mapDefParser->GetTDef(map.hardness, 100.0f, "MAP\\MapHardness");
	map.notDeformable = mapDefParser->SGetValueDef("0", "MAP\\NotDeformable") != "0";

	mapDefParser->GetTDef(map.gravity, 130.0f, "MAP\\Gravity");
	map.gravity = -map.gravity / (GAME_SPEED * GAME_SPEED);

	mapDefParser->GetTDef(map.tidalStrength, 0.0f, "MAP\\TidalStrength");
	mapDefParser->GetTDef(map.maxMetal, 0.02f, "MAP\\MaxMetal");
	mapDefParser->GetTDef(map.extractorRadius, 500.0f, "MAP\\ExtractorRadius");

	mapDefParser->GetDef(map.voidWater, "0", "MAP\\voidWater");
}


void CMapInfo::ReadGui()
{
	// GUI
	mapDefParser->GetTDef(gui.autoShowMetal, true, "MAP\\autoShowMetal");
}


void CMapInfo::ReadAtmosphere()
{
	// MAP\ATMOSPHERE
	mapDefParser->GetTDef(atmosphere.cloudDensity, 0.5f, "MAP\\ATMOSPHERE\\CloudDensity");
	mapDefParser->GetTDef(atmosphere.fogStart, 0.1f, "MAP\\ATMOSPHERE\\FogStart");
	atmosphere.fogColor = mapDefParser->GetFloat3(float3(0.7f, 0.7f, 0.8f), "MAP\\ATMOSPHERE\\FogColor");
	atmosphere.skyColor = mapDefParser->GetFloat3(float3(0.1f, 0.15f, 0.7f), "MAP\\ATMOSPHERE\\SkyColor");
	atmosphere.sunColor = mapDefParser->GetFloat3(float3(1.0f, 1.0f, 1.0f), "MAP\\ATMOSPHERE\\SunColor");
	atmosphere.cloudColor = mapDefParser->GetFloat3(float3(1.0f, 1.0f, 1.0f), "MAP\\ATMOSPHERE\\CloudColor");
	mapDefParser->GetTDef(atmosphere.minWind, 5.0f, "MAP\\ATMOSPHERE\\MinWind");
	mapDefParser->GetTDef(atmosphere.maxWind, 25.0f, "MAP\\ATMOSPHERE\\MaxWind");
	mapDefParser->GetDef(atmosphere.skyBox, "", "MAP\\ATMOSPHERE\\SkyBox");
}


void CMapInfo::ReadLight()
{
	// MAP\LIGHT
	light.sunDir = mapDefParser->GetFloat3(float3(0.0f, 1.0f, 2.0f), "MAP\\LIGHT\\SunDir");
	light.sunDir.Normalize();
	light.sunDir4[0] = light.sunDir[0];
	light.sunDir4[1] = light.sunDir[1];
	light.sunDir4[2] = light.sunDir[2];
	light.sunDir4[3] = 0.0f;

	light.groundAmbientColor = mapDefParser->GetFloat3(float3(0.5f, 0.5f, 0.5f), "MAP\\LIGHT\\GroundAmbientColor");
	light.groundSunColor = mapDefParser->GetFloat3(float3(0.5f, 0.5f, 0.5f), "MAP\\LIGHT\\GroundSunColor");
	light.groundSpecularColor = mapDefParser->GetFloat3(float3(0.1f, 0.1f, 0.1f), "MAP\\LIGHT\\GroundSpecularColor");
	mapDefParser->GetTDef(light.groundShadowDensity, 0.8f, "MAP\\LIGHT\\GroundShadowDensity");

	light.unitAmbientColor = mapDefParser->GetFloat3(float3(0.4f, 0.4f, 0.4f), "MAP\\LIGHT\\UnitAmbientColor");
	light.unitSunColor = mapDefParser->GetFloat3(float3(0.7f, 0.7f, 0.7f), "MAP\\LIGHT\\UnitSunColor");
	light.specularSunColor = mapDefParser->GetFloat3(light.unitSunColor, "MAP\\LIGHT\\SpecularSunColor");
	mapDefParser->GetTDef(light.unitShadowDensity, 0.8f, "MAP\\LIGHT\\UnitShadowDensity");
}


void CMapInfo::ReadWater()
{
	// MAP\WATER
	mapDefParser->GetTDef(water.repeatX, 0.0f, "MAP\\WATER\\WaterRepeatX");
	mapDefParser->GetTDef(water.repeatY, 0.0f, "MAP\\WATER\\WaterRepeatY");
	mapDefParser->GetTDef(water.damage, 0.0f, "MAP\\WATER\\WaterDamage");
	water.damage *= 16.0f / 30.0f;
	
	std::string tmp;
	mapDefParser->GetDef(tmp, "", "MAP\\WATER\\WaterPlaneColor");
	hasWaterPlane = !tmp.empty();
	water.planeColor = mapDefParser->GetFloat3(float3(0.0f, 0.4f, 0.0f), "MAP\\WATER\\WaterPlaneColor");

	water.surfaceColor = mapDefParser->GetFloat3(float3(0.75f, 0.8f, 0.85f), "MAP\\WATER\\WaterSurfaceColor");
	water.absorb = mapDefParser->GetFloat3(float3(0, 0, 0), "MAP\\WATER\\WaterAbsorb");
	water.baseColor = mapDefParser->GetFloat3(float3(0, 0, 0), "MAP\\WATER\\WaterBaseColor");
	water.minColor = mapDefParser->GetFloat3(float3(0, 0, 0), "MAP\\WATER\\WaterMinColor");
	mapDefParser->GetDef(water.texture, "", "MAP\\WATER\\WaterTexture");

	//default water is ocean.jpg in bitmaps, map specific water textures is saved in the map dir
	if(water.texture.empty())
		water.texture = "bitmaps/" + resources->SGetValueDef("ocean.jpg", "resources\\graphics\\maps\\watertex");
	else
		water.texture = "maps/" + water.texture;
}


void CMapInfo::ReadSmf()
{
	// SMF specific settings
	mapDefParser->GetDef(smf.detailTexName, "", "MAP\\DetailTex");
	if (smf.detailTexName.empty())
		smf.detailTexName = "bitmaps/" + resources->SGetValueDef("detailtex2.bmp", "resources\\graphics\\maps\\detailtex");
	else
		smf.detailTexName = "maps/" + smf.detailTexName;
}


void CMapInfo::ReadSm3()
{
	// SM3 specific settings
	sm3.minimap = mapDefParser->SGetValueDef("", "MAP\\minimap");
}


void CMapInfo::ReadTerrainTypes()
{
	for (int a = 0; a < 256; ++a) {
		char tname[200];
		sprintf(tname, "MAP\\TerrainType%i\\", a);
		std::string section = tname;
		mapDefParser->GetDef (terrainTypes[a].name,  "Default", section + "name");
		mapDefParser->GetTDef(terrainTypes[a].hardness,   1.0f, section + "hardness");
		mapDefParser->GetTDef(terrainTypes[a].tankSpeed,  1.0f, section + "tankmovespeed");
		mapDefParser->GetTDef(terrainTypes[a].kbotSpeed,  1.0f, section + "kbotmovespeed");
		mapDefParser->GetTDef(terrainTypes[a].hoverSpeed, 1.0f, section + "hovermovespeed");
		mapDefParser->GetTDef(terrainTypes[a].shipSpeed,  1.0f, section + "shipmovespeed");
		mapDefParser->GetDef(terrainTypes[a].receiveTracks, "1", section + "receivetracks");
	}
}
