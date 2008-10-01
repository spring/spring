//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#define BLD_FACTORY 8
#define BLD_TOTALBUILDSPACE 8

#define MAP_UPDATE_INTERVAL 16

class AIConfig;
class BuildTable;

// map properties for building - heightDif and midh are precalculated
struct MapInfo
{
	MapInfo() { heightDif=midh=0.0f; buildstate=0; numGeos=geosUsed=0; }

	float heightDif;
	float midh;

	ulong buildstate;
	int numGeos, geosUsed;
};

// dynamic properties
struct GameInfo
{
	GameInfo ()
	{ 
		lastLosFrame=0;
		threatProximity=losThreat=nthreat=0.0f;
		economicValue=enemyRange=threat=0.0f;

		localDefense=defense=defenseRange=0.0f;
	}

	int lastLosFrame; // last frame that sector appeared in los
	float nthreat; // used for updating - stores temporary threat value
	float losThreat;
	float threatProximity;
	float enemyRange;
	float economicValue;

	float damageAccumulator;
	int damageMeasureFrame;

	// actual threat based on:
	// - attacks on units
	// - controlled by ThreatDecay
	// - rangedThreat determines minimum value
	float threat;

	// threat based on losThreat and enemyRange of surrounding sectors
	float rangedThreat;

	float localDefense; // local defense in a sector
	float defense; // defense is based on localDefense&defenceRange
	float defenseRange; // range of the defense power given by localDefense

	std::vector <int> enemies;
};

class DbgWindow;

/*
holds the MapInfo map, GameInfo map and MetalSpotInfo map
*/
class InfoMap
{
public:
	InfoMap ();
	~InfoMap ();

	MapInfo* GetMapInfo (int x,int y) { return &mapinfo [y*mapw+x]; }
	MapInfo* GetMapInfo (int2 pos) { return &mapinfo [pos.y*mapw+pos.x]; }
	int2 GetMapInfoCoords (const float3& p);
	MapInfo* GetMapInfo (const float3& p) { return GetMapInfo (GetMapInfoCoords (p)); }

	// x and y are in gameinfo sector coords
	GameInfo* GetGameInfo (int x,int y) { return &gameinfo [y*gsw+x]; }
	GameInfo* GetGameInfo (int2 pos) { return &gameinfo [pos.y*gsw+pos.x]; }
	int2 GetGameInfoCoords (const float3& p); // get gameinfo from world coords
	GameInfo* GetGameInfoFromMapSquare (int x,int y);

	void CalculateInfoMap (IAICallback*, int BlockW);

	void UpdateThreatInfo (IAICallback*);
	void UpdateDefenseInfo (IAICallback*); // called by UpdateThreatInfo

	void UpdateBaseCenter (IAICallback *cb);

	// mapinfo sector
	int2 FindSafeBuildingSector (float3 sp, int maxdist,int reqspace);
	int2 FindDefenseBuildingSector (float3 sp, int maxdist);

	// gameinfo sector
	int2 FindAttackDefendSector (IAICallback*cb, int2 startpoint);

	void UpdateDamageMeasuring (float damage, int victim,IAICallback*cb);

	bool WriteThreatMap (const char *file);
	void DrawThreatMap (DbgWindow *wnd, int x,int y, int w,int h);
	void DrawDefenseMap (DbgWindow *wnd,int x,int y,int w,int h);

	int mblockw;
	int mblocksize;
	int mapw,maph;
	MapInfo *mapinfo;

	// game state blocks are twice the size of static map info blocks
	// gamestateInfow = mapw/2;
	// gamestateInfoh = maph/2;
	int gblockw, gblocksize;
	int gsw, gsh;
	GameInfo *gameinfo;

	uchar highestMetalValue;

	float2 baseCenter;
	float2 baseDirection; // 

	template<typename HF> int2 SelectGameInfoSector (HF hf)
	{
		float bestscore;
		int2 best (-1,0);

		for (int y=0;y<gsh;y++)
			for (int x=0;x<gsw;x++)
			{
				GameInfo& i=gameinfo[y*gsw+x];
				float score = hf(x,y,i);
				if (best.x < 0 || bestscore < score)
				{
					best=int2(x,y);
					bestscore=score;
				}
			}
		return best;
	}
};


struct SearchOffset {
	int dx,dy;
	int qdist; // dx*dx+dy*dy
};

extern int numSearchOffsets;
SearchOffset* GetSearchOffsetTable ();
void FreeSearchOffsetTable ();


