//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#include "BaseAIDef.h"
#include "InfoMap.h"
#include "BuildTable.h" // to access the cached unitdefs 
#include "DebugWindow.h"
#include "Sim/Features/FeatureDef.h"
#include "AI_Config.h"

// TODO: Fix enemy range mapping


#define NUM_DAMAGE_MEASURE_FRAMES 90

static const struct nOffset_t { int x,y; } nOffsetTbl[8] = {
	{ -1,-1 }, { 0, -1 }, { 1, -1 }, { -1, 0 }, { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 }
};

int numSearchOffsets=0;
static SearchOffset *searchOffsetTable = 0;

bool SearchOffsetComparator (const SearchOffset& a, const SearchOffset& b)
{
	return a.qdist < b.qdist;
}

SearchOffset* GetSearchOffsetTable ()
{
	const int w=256;

	if (!searchOffsetTable) {

		SearchOffset *tmp = new SearchOffset [w*w];

		for (int y=0;y<w;y++)
			for (int x=0;x<w;x++)
			{
				SearchOffset& i = tmp [y*w+x];

				i.dx = x-w/2;
				i.dy = y-w/2;
				i.qdist = i.dx*i.dx+i.dy*i.dy;
			}

		numSearchOffsets = w*w;
		sort (tmp, tmp+numSearchOffsets, SearchOffsetComparator);

		assert (tmp->qdist==0);

		searchOffsetTable=tmp;
	}
	return searchOffsetTable;
}

void FreeSearchOffsetTable ()
{
	if (searchOffsetTable)
		delete[] searchOffsetTable;
}

InfoMap::InfoMap ()
{
	gsw = gsh = 0;
	gameinfo = 0;

	mapw = maph = 0;
	mapinfo = 0;
	highestMetalValue = 0;

	gblockw = gblocksize = 0;
	mblockw = mblocksize = 0;
}

InfoMap::~InfoMap ()
{
	if (gameinfo)
		delete[] gameinfo;

	if (mapinfo)
		delete[] mapinfo;
}

int2 InfoMap::GetMapInfoCoords (const float3& p)
{
	int x = int(p.x)/mblocksize;
	int y = int(p.z)/mblocksize;
	if (x < 0) x=0; if (y < 0) y = 0;
	if (x >= mapw) x = mapw-1; 
	if (y >= maph) y = maph-1;
	return int2(x,y);
}

int2 InfoMap::GetGameInfoCoords (const float3& p)
{
	int x = int(p.x)/gblocksize;
	int y = int(p.z)/gblocksize;
	if (x < 0) x=0; if (y < 0) y = 0;
	if (x >= gsw) x = gsw-1; 
	if (y >= gsh) y = gsh-1;
	return int2(x,y);
}

GameInfo* InfoMap::GetGameInfoFromMapSquare (int x,int y)
{
	x /= gblockw;
	y /= gblockw;
	if (x < 0) x=0;
	if (y < 0) y=0;
	if (x>=gsw) x=gsw-1;
	if (y>=gsh) y=gsh-1;
	return  &gameinfo[y*gsw+x];
}

int2 InfoMap::FindDefenseBuildingSector (float3 sp, int maxdist)
{
	int2 bestsector(-1,0);
	float bestscore;
	SearchOffset *tbl = GetSearchOffsetTable ();
	int2 sector = GetMapInfoCoords (sp);

	for (int a=0;a<numSearchOffsets;a++)
	{
		SearchOffset& ofs = tbl [a];
		if (ofs.qdist >= maxdist*maxdist)
			break;

		int x = ofs.dx + sector.x;
		int y = ofs.dy + sector.y;
		if (x >= mapw || y >= maph || x < 0 || y < 0)
			continue;
		MapInfo& blk = mapinfo [y*mapw+x];
		if (blk.midh < 0.0f)
			continue;
		if (blk.buildstate)
			continue;

		// higher score means better sector
		GameInfo &gi = gameinfo [y/2*gsw+x/2];
		float score = 1.0f / (1.0f + gi.defense) / (1.0f + sqrtf(ofs.qdist));

		if (bestsector.x < 0 || bestscore < score)
		{
			bestscore = score;
			bestsector = int2 (x,y);
		}
	}
	return bestsector;
}

int2 InfoMap::FindSafeBuildingSector (float3 sp, int maxdist, int reqspace)
{
	int qd = maxdist*maxdist;
	SearchOffset *tbl = GetSearchOffsetTable ();
	int2 sector = GetMapInfoCoords (sp);

	float bestscore;
	int2 bestsector (-1,0);

	for (int a=0;a<numSearchOffsets;a++)
	{
		SearchOffset& ofs = tbl [a];

		if (ofs.qdist >= maxdist*maxdist)
			break;

		int x = ofs.dx + sector.x;
		int y = ofs.dy + sector.y;

		if (x >= mapw || y >= maph || x < 0 || y < 0)
			continue;

		MapInfo& blk = mapinfo [y*mapw+x];

		if (blk.midh < 0.0f)
			continue;

		if (blk.buildstate + reqspace > BLD_TOTALBUILDSPACE)
			continue;

	//	if (blk.heightDif > highestMetalValue / 2 + highestMetalValue / 4 && surr < 3)
	//		return int2 (x,y);

		// lower score means better sector
		GameInfo &gi = gameinfo [y/2*gsw+x/2];
		float score = sqrtf (ofs.qdist) * blk.heightDif * gi.threat;

		float dx= baseCenter.x/mblocksize-x, dy= baseCenter.y/mblocksize-y;
		score *= max(1.0f, sqrtf (dx*dx+dy*dy));// + (baseDirection.x * ofs.dx + baseDirection.y * ofs.dy));

		if (bestsector.x < 0 || bestscore > score)
		{
			bestscore = score;
			bestsector = int2 (x,y);
		}
	}

	MapInfo *b=GetMapInfo(bestsector);
//	logPrintf ("RequestSafeBuildingZone(): heightdif=%f\n", b->heightDif);
	return bestsector;
}

int2 InfoMap::FindAttackDefendSector (IAICallback *cb, int2 startpoint)
{
	float bestscore;
	int2 best (-1,0);

	// picks the lowest score
	for (int y=0;y<gsh;y++)
		for (int x=0;x<gsw;x++) {
			GameInfo& i = gameinfo [y*gsw+x];

			if (!i.losThreat)
				continue;

			// negative economic value is enemy property
			float score = max(1.0f, -i.economicValue) / max(1.0f, i.losThreat); // higher threat is good
			int dx=x-startpoint.x,dy=y-startpoint.y;
			score *= max(1.0f,sqrtf (dx*dx+dy*dy)); // longer distance is worse

			if (best.x < 0 || bestscore > score)
			{
				best=int2(x,y);
				bestscore=score;
			}
		}

	return best;
}


void InfoMap::UpdateBaseCenter (IAICallback *cb)
{
	int numUnits = cb->GetFriendlyUnits (idBuffer);

	double sumX=0.0, sumZ=0.0;
	int num=0;
	bool fixedOnly=true;

	if (!numUnits)
		return;
	
	while (1)
	{
		for (int a=0;a<numUnits;a++)
		{
			int u = idBuffer [a];

			const UnitDef* def = cb->GetUnitDef (u);
			if (def->canmove && fixedOnly)
				continue;

			if (cb->GetUnitTeam (u) == cb->GetMyTeam())
			{
				float3 pos = cb->GetUnitPos (u);
				GameInfo* gi = GetGameInfo (GetGameInfoCoords(pos));

				sumX += pos.x;
				sumZ += pos.z;
				num ++;
			}
		}

		if (num > 0) {
			sumX /= num;
			sumZ /= num;

			baseCenter.x = sumX;
			baseCenter.y = sumZ;
			
			baseDirection.x = baseDirection.y = 0.0f;

			for (int a=0;a<numUnits;a++)
			{
				int u = idBuffer [a];

				const UnitDef* def = cb->GetUnitDef (u);
				if (def->canmove && fixedOnly)
					continue;

				if (cb->GetUnitTeam (u) == cb->GetMyTeam())
				{
					float3 pos = cb->GetUnitPos (u);
					GameInfo* gi = GetGameInfo (GetGameInfoCoords(pos));

					baseDirection.x += fabs(pos.x - baseCenter.x);
					baseDirection.y += fabs(pos.y - baseCenter.y);
				}
			}

			break;
		}
		if (!fixedOnly && !num)
			break;

		num=0;
		fixedOnly=false;
	}
}

void InfoMap::UpdateDamageMeasuring (float damage, int victim, IAICallback *cb)
{
	float3 pos = cb->GetUnitPos (victim);
	int2 sector = GetGameInfoCoords(pos);
	GameInfo& gi = gameinfo[sector.y*gsw+sector.y];

	if (gi.damageMeasureFrame < 0) {
		gi.damageMeasureFrame = cb->GetCurrentFrame();

		gi.damageAccumulator = damage;
	} else
		gi.damageAccumulator += damage;
}

void InfoMap::UpdateThreatInfo (IAICallback *cb)
{
	int curFrame = cb->GetCurrentFrame ();

	const ushort *losmap = cb->GetLosMap ();
	const ushort *radarmap = cb->GetRadarMap ();

	// clear new threats
	GameInfo* b=gameinfo;
	for (int y=0;y<gsh;y++)
		for (int x=0;x<gsw;x++)
		{
			b->enemies.clear ();
			b->rangedThreat = 0;
			b->economicValue = 0;
			b->defense = 0;
			b->defenseRange = 0;
			b->localDefense = 0;
			b->enemyRange = 0.0f;
			(b++)->nthreat = 0;
		}

	// iterate through all enemy units
	int numUnits = cb->GetEnemyUnits (idBuffer);
	for (int a=0;a<numUnits;a++)
	{
		int u = idBuffer [a];
		float3 pos = cb->GetUnitPos (u);
		int2 sp = GetGameInfoCoords (pos);

		GameInfo& blk = gameinfo[sp.y*gsw+sp.x];
		const UnitDef* def = cb->GetUnitDef (u);
		BuildTable::UDef *ud = buildTable.GetCachedDef (def->id);

		blk.enemies.push_back (u);
		blk.nthreat += ud->weaponDamage;
		blk.economicValue -= cb->GetUnitPower (u);

		if (blk.enemyRange < ud->weaponRange)
			blk.enemyRange = ud->weaponRange;
	}

	// iterate through all friendly units
	numUnits = cb->GetFriendlyUnits (idBuffer);
	for (int a=0;a<numUnits;a++)
	{
		int u = idBuffer [a];
		float3 pos = cb->GetUnitPos (u);
		int2 sp = GetGameInfoCoords (pos);

		GameInfo& blk = gameinfo[sp.y*gsw+sp.x];
		const UnitDef* def = cb->GetUnitDef (u);
		BuildTable::UDef *ud = buildTable.GetCachedDef (def->id);

		blk.economicValue += cb->GetUnitPower (u);

		if (ud->IsBuilding()) {
			blk.localDefense += ud->weaponDamage;
			if (blk.defenseRange < ud->weaponRange)
				blk.defenseRange = ud->weaponRange;
		}
	}

	int lm = gblockw/2, losw=cb->GetMapWidth ()/2;
	int rm = gblockw/8, radarw=cb->GetMapWidth ()/8;

	SearchOffset *sofs = GetSearchOffsetTable();

	for (int y=0;y<gsh;y++)
		for (int x=0;x<gsw;x++)
		{
			GameInfo &gi=gameinfo[y*gsw+x];

			if (!gi.nthreat && !gi.localDefense)
				continue;

			// calculate enemy threat range
			int enemyRange = (int) ceilf(gi.enemyRange / gblocksize);
			// calculate defense range
			int defenseRange = (int) ceilf(gi.defenseRange / gblocksize);
			int range = max(enemyRange, defenseRange);
			int minx = max(x-range,0), maxx = min(x+range,gsw);
			int miny = max(y-range,0), maxy = min(y+range,gsh);

			range *= range;
			enemyRange *= enemyRange;
			defenseRange *= defenseRange;

			for (int sy=miny;sy<maxy;sy++) 
				for (int sx=minx;sx<maxx;sx++)
				{
					int dx=sx-x,dy=sy-y;
					dx=dx*dx+dy*dy;

					if (dx <= enemyRange)
						gameinfo[sy*gsw+sx].rangedThreat += gi.nthreat;
					if (dx <= defenseRange)
						gameinfo[sy*gsw+sx].defense += gi.localDefense;
				}
		}

	for (int y=0;y<gsh;y++)
		for (int x=0;x<gsw;x++)
		{
			GameInfo &b = gameinfo[y*gsw+x];
			if (losmap [(lm*y+lm/2)*losw+x*lm+lm/2])
			{
				b.lastLosFrame = curFrame;
				b.losThreat = b.nthreat;
			}

			// handle threat measuring with UnitDamaged()
			if (b.damageMeasureFrame >= 0 && b.damageMeasureFrame + NUM_DAMAGE_MEASURE_FRAMES < curFrame)
			{
				int statisticDamage = (int) b.damageAccumulator / 6; // amount of damage per half second - might need tuning
				if (b.threat < statisticDamage)
					b.threat = statisticDamage;

				b.damageMeasureFrame = -1;
			}

			// use rangedThreat as a minimum value
			if (b.threat < b.rangedThreat)
				b.threat = b.rangedThreat;
		}
}


void InfoMap::CalculateInfoMap (IAICallback *cb, int BlockW)
{
	const float* hm = cb->GetHeightMap();
	int hmw = cb->GetMapWidth ();
	int hmh = cb->GetMapHeight ();

	// Calculate static info map size
	mblockw = BlockW;
	mblocksize = SQUARE_SIZE * BlockW;
	mapw = hmw / mblockw;
	maph = hmh / mblockw;
	mapinfo = new MapInfo [mapw*maph];

	// Calculate dynamic game state map size
	gblockw = BlockW * 2;
	gblocksize = SQUARE_SIZE * BlockW * 2;
	gsw = hmw / gblockw;
	gsh = hmh / gblockw;
	gameinfo = new GameInfo [gsw*gsh];

	ChatDebugPrintf(cb, "MapInfo: %dx%d, GameStateInfo: %dx%d\n", mapw,maph,gsw,gsh);

	for (int my=0;my<maph;my++)
	{
		for (int mx=0;mx<mapw;mx++)
		{
			float minh,maxh;
			double sum=0.0;

			minh=maxh=hm[mblockw*(my*hmw+mx)];

			for (int y=mblockw*my;y<mblockw*(my+1);y++)
				for (int x=mblockw*mx;x<mblockw*(mx+1);x++)
				{
					float h = hm [hmw * y + x];
					sum += h;

					if (h < minh) minh=h;
					if (h > maxh) maxh=h;
				}

			MapInfo& info = mapinfo[mapw*my+mx];

			info.midh = sum / (mblockw*mblockw);
			info.heightDif = std::max (info.midh - minh, maxh - info.midh);
		}
	}

	vector <int> features;
	int nf;

	features.resize (5000);
	do {
		features.resize (features.size()*2);
		nf = cb->GetFeatures (&features[0], features.size());
	} while (nf == features.size());

	for(int a=0;a<nf;a++)
	{
		const FeatureDef* fd = cb->GetFeatureDef(features[a]);

		if (fd->geoThermal)
		{
			float3 pos = cb->GetFeaturePos(features[a]);
			int2 c = GetGameInfoCoords(pos);

			
		}
	}
}

/*
Saves a 24 bit uncompressed TGA image
*/
bool InfoMap::WriteThreatMap (const char *file)
{
	// open file
	FILE *fp=fopen(file, "wb");
	if(!fp)
		return false;

	// fill & write header
	char targaheader[18];
	memset(targaheader, 0, sizeof(targaheader));

	targaheader[2] = 2;		/* image type = uncompressed gray-scale */
	targaheader[12] = (char) (gsw & 0xFF);
	targaheader[13] = (char) (gsw >> 8);
	targaheader[14] = (char) (gsh & 0xFF);
	targaheader[15] = (char) (gsh >> 8);
	targaheader[16] = 24; /* 24 bit color */
	targaheader[17] = 0x20;	/* Top-down, non-interlaced */

	fwrite(targaheader, 18, 1, fp);

	// write file
	uchar out[3];

	GameInfo *gi = gameinfo;
	for (int y=0;y<gsh;y++)
		for (int x=0;x<gsw;x++)
		{
			const int div=4;
			out[0]=min(255,(int)gi->threat / div);
			out[1]=min(255,(int)gi->losThreat / div);
			out[2]=min(255,(int)gi->rangedThreat / div);

			fwrite (out, 3, 1, fp);
			gi++;
		}
	fclose(fp);

	return true;
}

static uchar data [10000];

void InfoMap::DrawThreatMap (DbgWindow *wnd,int sx,int sy,int sw,int sh)
{
	if (gsw * gsh * 3 > sizeof(data))
		return;

	GameInfo *gi = gameinfo;
	uchar *out = data;
	for (int y=0;y<gsh;y++)
		for (int x=0;x<gsw;x++)
		{
			const int div=4;
			out[0]=min(255,(int)gi->threat / div);
			out[1]=min(255,(int)gi->losThreat / div);
			out[2]=min(255,(int)gi->rangedThreat / div);

			out += 3;
			gi++;
		}

	DbgWindowStretchBitmap (wnd, sx, sy, sw, sh, gsw,gsh, (char *)data);
}

void InfoMap::DrawDefenseMap (DbgWindow *wnd, int sx,int sy,int sw,int sh)
{
	if (gsw * gsh * 3 > sizeof(data))
		return;

	GameInfo *gi = gameinfo;
	uchar *out = data;
	for (int y=0;y<gsh;y++)
		for (int x=0;x<gsw;x++)
		{
			const int div=4;
			out[0]=min(255,(int)gi->threat / div);
			out[1]=min(255,(int)gi->localDefense / div);
			out[2]=min(255,(int)gi->defense / div);

			out += 3;
			gi++;
		}

	DbgWindowStretchBitmap (wnd, sx, sy, sw, sh, gsw,gsh, (char *)data);
}
