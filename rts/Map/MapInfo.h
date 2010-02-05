#ifndef MAPINFO_H
#define MAPINFO_H

#include <string>
#include <vector>
#include "float3.h"
#include "float4.h"

class LuaTable;
class MapParser;

class CMapInfo
{
public:

	CMapInfo(const std::string& mapName);
	void Load(); // fill in infos
	~CMapInfo();

	/* The settings are just public members because:

	   1) it's quite some work to encapsulate all of them, and
	   2) nothing too bad happens if you modify them, there are no complex
	      pointer members that really beg for encapsulation.

	   Instead of encapsulation it is as effective and much easier make the
	   global mapInfo const, ie. const CMapInfo* mapInfo;
	 */

	/* Note: this could (should) have been anonymous structures if only MSVC 8
	   didn't crap out on it.  Specifically, it craps out on any class with at
	   least 1 user defined non-inline constructor and at least 2 anonymous
	   structures, each with at least 1 object with a constructor in it.
	   In other words, it probably assigns the same name to each anonymous
	   structure, and later gets confused by that.

	   This sample code triggers the problem in MSVC:

		class A {
			A::A();
			struct { std::string s1; } a1;
			struct { std::string s2; } a2;
		};
		A::A() {}
	 */

	std::string GetStringValue(const std::string& key) const; // can be used before Load()

	/** Global settings, ie. from "MAP" section. */
	struct map_t {
		std::string name;      ///< The filename as passed to the constructor.
		std::string humanName; ///< "MAP\\Description"
		std::string author;
		float hardness;        ///< "MAP\\MapHardness"
		bool  notDeformable;
		/** Stores the gravity as a negative number in units/frame^2
		    (NOT positive units/second^2 as in the mapfile) */
		float gravity;
		float tidalStrength;
		/// what metal value 255 in the metal map is worth
		float maxMetal;
		float extractorRadius; ///< extraction radius for mines
		bool  voidWater;
	} map;

	/** GUI settings (used by CGuiHandler) */
	struct gui_t {
		bool autoShowMetal;
	} gui;

	/** settings read from "MAP\ATMOSPHERE" section */
	struct atmosphere_t {
		float  cloudDensity;
		float  fogStart;
		float4 fogColor;
		float3 skyColor;
		float3 sunColor;
		float3 cloudColor;
		float  minWind;
		float  maxWind;
		std::string skyBox;
	} atmosphere;

	/** settings read from "MAP\LIGHT" section */
	struct light_t {
		float4 sunDir;     ///< Holds vector for the direction of the sun
		float3 groundAmbientColor;
		float3 groundSunColor;
		float3 groundSpecularColor;
		float  groundShadowDensity;
		float4 unitAmbientColor;
		float4 unitSunColor;
		float  unitShadowDensity;
		float3 unitSpecularColor;
	} light;

	/** settings read from "MAP\WATER" section
	    prefix their name with "Water" to get the TDF variable */
	struct water_t {
		float  repeatX; ///< (calculated default is in CBaseWater)
		float  repeatY; ///< (calculated default is in CBaseWater)
		float  damage;
		float3 absorb;
		float3 baseColor;
		float3 minColor;
		float3 surfaceColor;
		float  surfaceAlpha;
		float4 planeColor;
		float3 diffuseColor;
		float3 specularColor;
		float  ambientFactor;
		float  diffuseFactor;
		float  specularFactor;
		float  specularPower;
		float  fresnelMin;
		float  fresnelMax;
		float  fresnelPower;
		float  reflDistortion;
		float  blurBase;
		float  blurExponent;
		float  perlinStartFreq;
		float  perlinLacunarity;
		float  perlinAmplitude;
		float  windSpeed;
		bool   shoreWaves;
		bool   forceRendering; ///< if false the renderers will render it only if currentMinMapHeight<0
		bool   hasWaterPlane;  ///< true if "MAP\WATER\WaterPlaneColor" is set
		unsigned char numTiles;
		std::string texture;
		std::string foamTexture;
		std::string normalTexture;
		std::vector<std::string> causticTextures;
	} water;

	/** SMF specific settings */
	struct smf_t {
		std::string detailTexName; ///< "MAP\DetailTex"
		std::string specularTexName; ///< "MAP\SpecularTex"

		float minHeight;
		bool  minHeightOverride;
		float maxHeight;
		bool  maxHeightOverride;

		std::vector<std::string> smtFileNames;
	} smf;

	/** SM3 specific settings
	    This is NOT complete, SM3 stores so much in the map settings
	    that it really isn't a good idea to put them here. */
	struct sm3_t {
		std::string minimap; ///< "MAP\minimap"
	} sm3;

	/** Terrain type, there can be 256 of these:
	    "MAP\TerrainType0" up to "MAP\TerrainType255" */
	static const int NUM_TERRAIN_TYPES = 256;
	struct TerrainType {
		std::string name;
		float hardness;
		float tankSpeed;   ///< "TankMoveSpeed"
		float kbotSpeed;   ///< "KbotMoveSpeed"
		float hoverSpeed;  ///< "HoverMoveSpeed"
		float shipSpeed;   ///< "ShipMoveSpeed"
		bool receiveTracks;
	};
	TerrainType terrainTypes[NUM_TERRAIN_TYPES];

private:
	void ReadGlobal();
	void ReadGui();
	void ReadAtmosphere();
	void ReadLight();
	void ReadWater();
	void ReadSmf();
	void ReadSm3();
	void ReadTerrainTypes();

	MapParser* parser; // map       parser root table
	LuaTable* resRoot; // resources parser root table
};

extern const CMapInfo* mapInfo;

#endif // MAPINFO_H
