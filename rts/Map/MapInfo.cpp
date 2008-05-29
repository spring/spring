#include "MapInfo.h"

#include "Lua/LuaParser.h"
#include "System/LogOutput.h"
#include "System/TdfParser.h"
#include "System/FileSystem/FileHandler.h"

float4::float4()
{
	float tmp[4];

	// ensure alignment is correct to use it as array of floats
	(void) tmp;
	assert(&y - &x == &tmp[1] - &tmp[0]);
	assert(&z - &x == &tmp[2] - &tmp[0]);
	assert(&w - &x == &tmp[3] - &tmp[0]);

	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
	w = 0.0f;
}


// before delete, the const is const_cast'ed away.
// there are no (other) situations where mapInfo may be modified
const CMapInfo* mapInfo;


CMapInfo::CMapInfo(const std::string& mapname)
{
	map.name = mapname;
	mapDefParser = new TdfParser(GetTDFName(mapname));
	resourcesParser = new LuaParser ("gamedata/resources.lua",
	                                 SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	if (!resourcesParser->Execute()) {
		logOutput.Print(resourcesParser->GetErrorLog());
	}
	
	ReadGlobal();
	ReadAtmosphere();
	ReadGui();
	ReadLight();
	ReadWater();
	ReadSmf();
	ReadSm3();
	ReadTerrainTypes();

	delete resourcesParser;
	resourcesParser = NULL;
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
	map.wantedScript = mapDefParser->SGetValueDef(map.wantedScript, "MAP\\Script");

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

	light.groundAmbientColor = mapDefParser->GetFloat3(float3(0.5f, 0.5f, 0.5f), "MAP\\LIGHT\\GroundAmbientColor");
	light.groundSunColor = mapDefParser->GetFloat3(float3(0.5f, 0.5f, 0.5f), "MAP\\LIGHT\\GroundSunColor");
	light.groundSpecularColor = mapDefParser->GetFloat3(float3(0.1f, 0.1f, 0.1f), "MAP\\LIGHT\\GroundSpecularColor");
	mapDefParser->GetTDef(light.groundShadowDensity, 0.8f, "MAP\\LIGHT\\GroundShadowDensity");

	light.unitAmbientColor = float4(mapDefParser->GetFloat3(float3(0.4f, 0.4f, 0.4f), "MAP\\LIGHT\\UnitAmbientColor"), 1.0f);
	light.unitSunColor = float4(mapDefParser->GetFloat3(float3(0.7f, 0.7f, 0.7f), "MAP\\LIGHT\\UnitSunColor"), 1.0f);
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

	mapDefParser->GetDef(water.fresnelMin,"0.2","MAP\\WATER\\FresnelMin");
	mapDefParser->GetDef(water.fresnelMax,"0.3","MAP\\WATER\\FresnelMax");
	mapDefParser->GetDef(water.fresnelPower,"4.0","MAP\\WATER\\FresnelPower");

	mapDefParser->GetDef(water.specularFactor,"20.0","MAP\\WATER\\WaterSpecularFactor");

	water.specularColor = mapDefParser->GetFloat3(light.groundSunColor,"MAP\\WATER\\WaterSpecularColor");
	water.surfaceColor = mapDefParser->GetFloat3(float3(0.75f, 0.8f, 0.85f), "MAP\\WATER\\WaterSurfaceColor");
	mapDefParser->GetDef(water.surfaceAlpha,"0.55","MAP\\WATER\\WaterSurfaceAlpha");
	water.absorb = mapDefParser->GetFloat3(float3(0, 0, 0), "MAP\\WATER\\WaterAbsorb");
	water.baseColor = mapDefParser->GetFloat3(float3(0, 0, 0), "MAP\\WATER\\WaterBaseColor");
	water.minColor = mapDefParser->GetFloat3(float3(0, 0, 0), "MAP\\WATER\\WaterMinColor");

	mapDefParser->GetDef(water.texture, "", "MAP\\WATER\\WaterTexture");
	mapDefParser->GetDef(water.foamTexture, "", "MAP\\WATER\\WaterFoamTexture");
	mapDefParser->GetDef(water.normalTexture, "", "MAP\\WATER\\WaterNormalTexture");

	//default water is ocean.jpg in bitmaps, map specific water textures is saved in the map dir
	const LuaTable mapsTable = resourcesParser->GetRoot().SubTable("graphics").SubTable("maps");
	const LuaTable causticsTable = resourcesParser->GetRoot().SubTable("graphics").SubTable("caustics");
	
	if(water.texture.empty())
		water.texture = "bitmaps/" + mapsTable.GetString("watertex", "ocean.jpg");
	else
		water.texture = "maps/" + water.texture;

	if(water.foamTexture.empty())
		water.foamTexture = "bitmaps/" + mapsTable.GetString("waterfoamtex", "foam.jpg");
	else
		water.foamTexture = "maps/" + water.foamTexture;

	if(water.normalTexture.empty())
		water.normalTexture = "bitmaps/" + mapsTable.GetString("waternormaltex", "waterbump.png");
	else
		water.normalTexture = "maps/" + water.normalTexture;

	char num[10];
	for (int i = 0; i < causticTextureCount; i++) {
		sprintf(num, "%02i", i);
		water.causticTextures[i] = std::string("bitmaps/") + causticsTable.GetString(i+1, 
															 std::string("caustics/caustic")+num+".jpg");
	}
}


void CMapInfo::ReadSmf()
{
	// SMF specific settings
	mapDefParser->GetDef(smf.detailTexName, "", "MAP\\DetailTex");

	const LuaTable mapsTable = resourcesParser->GetRoot().SubTable("graphics").SubTable("maps");
	
	if (smf.detailTexName.empty())
		smf.detailTexName = "bitmaps/" + mapsTable.GetString("detailtex","detailtex2.bmp");
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
