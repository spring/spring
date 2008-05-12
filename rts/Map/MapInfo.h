#ifndef MAPINFO_H
#define MAPINFO_H

#include <string>
#include "float3.h"

class TdfParser;


class CMapInfo
{
public:

	static void OpenTDF(const std::string& mapname, TdfParser& parser);
	static std::string GetTDFName(const std::string& mapname);

	CMapInfo(const std::string& mapname);
	~CMapInfo();

	/** @brief Get a readonly reference to the TDF parser.
	    This is needed by SM3 code to load feature and layer data. */
	const TdfParser& GetMapDefParser() const { return *mapDefParser; }

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

	/** Global settings, ie. from "MAP" section. */
	struct map_t {
		std::string name;      ///< The filename as passed to the constructor.
		std::string wantedScript;
		std::string humanName; ///< "MAP\\Description"
		float hardness;        ///< "MAP\\MapHardness"
		bool  notDeformable;
		/** Stores the gravity as a negative number in units/frame^2
		    (NOT positive units/second^2 as in the mapfile) */
		float gravity;
		float tidalStrength;
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
		float3 fogColor;
		float3 skyColor;
		float3 sunColor;
		float3 cloudColor;
		float  minWind;
		float  maxWind;
		std::string skyBox;
	} atmosphere;

	/** settings read from "MAP\LIGHT" section */
	struct light_t {
		float3 sunDir;     ///< Holds vector for the direction of the sun
		float  sunDir4[4]; ///< sunDir as 4 component vector
		float3 groundAmbientColor;
		float3 groundSunColor;
		float3 groundSpecularColor;
		float  groundShadowDensity;
		float3 unitAmbientColor;
		float3 unitSunColor;
		float3 specularSunColor;
		float  unitShadowDensity;
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
		float surfaceAlpha;
		float3 planeColor;
		float3 specularColor;
		float specularFactor;
		float fresnelMin;
		float fresnelMax;
		float fresnelPower;
		std::string texture;
		std::string foamTexture;
		std::string normalTexture;
		std::string causticTextures[32];
	} water;
	bool hasWaterPlane; ///< true if "MAP\WATER\WaterPlaneColor" is set

	/** SMF specific settings */
	struct smf_t {
		std::string detailTexName; ///< "MAP\DetailTex"
	} smf;

	/** SM3 specific settings
	    This is NOT complete, SM3 stores so much in the map settings
	    that it really isn't a good idea to put them here. */
	struct sm3_t {
		std::string minimap; ///< "MAP\minimap"
	} sm3;

	/** Terrain type, there can be 256 of these:
	    "MAP\TerrainType0" up to "MAP\TerrainType255" */
	struct TerrainType {
		std::string name;
		float hardness;
		float tankSpeed;   ///< "TankMoveSpeed"
		float kbotSpeed;   ///< "KbotMoveSpeed"
		float hoverSpeed;  ///< "HoverMoveSpeed"
		float shipSpeed;   ///< "ShipMoveSpeed"
		bool receiveTracks;
	};
	TerrainType terrainTypes[256];

private:

	void ReadGlobal();
	void ReadGui();
	void ReadAtmosphere();
	void ReadLight();
	void ReadWater();
	void ReadSmf();
	void ReadSm3();
	void ReadTerrainTypes();

	TdfParser* resources;
	TdfParser* mapDefParser;
};

extern const CMapInfo* mapInfo;

#endif // MAPINFO_H
