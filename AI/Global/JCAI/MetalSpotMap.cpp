//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
#include "BaseAIDef.h"
#include "MetalSpotMap.h"
#include "InfoMap.h"

// note that currently, the metal map info caching is disabled because it takes very little time to generate

#define MMAPCACHE_VERSION 1

static const int BlockWidth=32;
static const int2 SpotOffset(0,0);

static const struct Offset_t { int x,y; } OffsetTbl[8] = {
	{ -1,-1 }, { 0, -1 }, { 1, -1 }, { -1, 0 }, { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 }
};


MetalSpotMap::MetalSpotMap()
{
	averageProduction=0.0f;
	debugShowSpots=false;
	blocks = 0;
	width=height=0;
}

MetalSpotMap::~MetalSpotMap()
{
	if (blocks) delete[] blocks;
}

int MetalSpotMap::GetNumSpots() { return spots.size(); }

// TODO: Optimize using the blocks
MetalSpotID MetalSpotMap::FindSpot(const float3& startpos, InfoMap* infoMap, float extractDepth, bool water)
{
	float bestdis;
	int best=-1;

	for (int a=0;a<spots.size();a++) {
		MetalSpot& s = spots[a];
		if (s.extractDepth < extractDepth)
		{
			int dx = s.pos.x*SQUARE_SIZE - (int) startpos.x;
			int dy = s.pos.y*SQUARE_SIZE - (int) startpos.z;
			GameInfo *gi = infoMap->GetGameInfoFromMapSquare (s.pos.x, s.pos.y);
			float dis=sqrtf(dx*dx+dy*dy) * (20+gi->threat);
			if(best<0 || dis < bestdis) {
				bestdis=dis;
				best=a;
			}
		}
	}
	return best;
}

/*
MetalSpotID MetalSpotMap::FindSpot(const float3& startpos, InfoMap *infoMap, float extractDepth, bool water)
{
	SearchOffset *sofs = GetSearchOffsetTable ();

	int sx = startpos.x / (BlockWidth * SQUARE_SIZE);
	int sy = startpos.z / (BlockWidth * SQUARE_SIZE);

	float bestScore;
	int dist=-1;
	int best=-1;

	for (int a=0;a<numSearchOffsets;a++) {
		int x=sofs->dx + sx;
		int y=sofs->dy + sy;

		if (x<0 || y<0 || x>=width || y>=height) 
			continue;

		if (dist>=0)
		{
			if (sofs->qdist-10 > dist)
				break;
		}

		Block *pb = &blocks[y*width+x];
		for (int i=pb->first;i<pb->count+pb->first;i++)
		{
			if (((spots[i].height > 0) ^ water) && spots[i].extractDepth < extractDepth) {
				GameInfo* ginfo = infoMap->GetGameInfoFromMapSquare (spots[i].pos.x,spots[i].pos.y);
				float score = (extractDepth - spots[i].extractDepth ) / (1.0f + ginfo->threat);

				if (best<0 || bestScore < score) {
					bestScore = score;
					best = i;
					if (dist<0) dist = sofs->qdist;
				}
			}
		}
		sofs++;
	}

	return best;
}*/

void MetalSpotMap::SetSpotExtractDepth (MetalSpotID spot, float extraction) {
	spots[spot].extractDepth=extraction;

	if (!extraction)
		spots[spot].unit = 0;
}

static bool SpotCompare(const MetalSpot& a, const MetalSpot& b) { return a.block < b.block; }

void MetalSpotMap::CalcBlocks (IAICallback *cb)
{
	width = (int) ceil(cb->GetMapWidth () / float(BlockWidth));
	height = (int) ceil(cb->GetMapHeight () / float(BlockWidth));

	blocks = new Block[width*height];

	for (int a=0;a<spots.size();a++) {
		int x=spots[a].pos.x/BlockWidth;
		int y=spots[a].pos.y/BlockWidth;
		spots[a].block = y*width+x;
		spots[a].unit=0;
	}

	std::sort(spots.begin(),spots.end(),SpotCompare);
	int curBlock=0;
	for (int a=0;a<spots.size();a++) {
		if (curBlock != spots[a].block) {
			//logPrintf ("Block %d,%d has %d spots.\n", curBlock % BlockWidth, curBlock/BlockWidth, blocks[curBlock].count);
			curBlock = spots[a].block;
			blocks[curBlock].first = a;
			blocks[curBlock].count = 1;
		} else
			blocks[curBlock].count ++;
	}
}

#define METAL_MAP_SQUARE_SIZE (SQUARE_SIZE*2)

static bool metalcmp(float3 a, float3 b) { return a.y > b.y; }

			

static void Calc2DBuildPos(float3& pos, const UnitDef* ud)
{
	if(ud->xsize&2)
		pos.x=floor((pos.x)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.x=floor((pos.x+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;

	if(ud->ysize&2)
		pos.z=floor((pos.z)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.z=floor((pos.z+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;
}

void MetalSpotMap::Initialize (IAICallback *cb) //add isMetalmap true?
{	
	const float* heightMap = cb->GetHeightMap();
	int hmw = cb->GetMapWidth ();
	int hmh = cb->GetMapHeight ();

	float exRadius = cb->GetExtractorRadius () / METAL_MAP_SQUARE_SIZE;
	bool isMetalMap = false;

	int metalblockw = (int) (exRadius*2);
	if (metalblockw < 3) {
		metalblockw = 3; // to support metal maps
		isMetalMap = true; // kinda hackish, but who cares if it works :)
	}
	int mapblockw = metalblockw*2;

	int w = 1 + hmw / mapblockw;
	int h = 1 + hmh / mapblockw;

	logPrintf ("Extractor Radius: %f, mapblockw=%d, metalblockw=%d\n", exRadius, w,h,mapblockw,metalblockw);

	const unsigned char* metalmap=cb->GetMetalMap ();

	uchar bestspot=0;
	ulong totalMetal=0;

	for (const uchar *mm = metalmap; mm < metalmap + (hmw*hmh/4); mm++)
		totalMetal += *mm;

	averageMetal = totalMetal / float(hmw * hmh / 4);
	int minMetalOnSpot = 50;

	for (int my=0;my<h;my++)
	{
		for (int mx=0;mx<w;mx++)
		{
			float minh,maxh;
			double sum=0.0;

			int endy = std::min(mapblockw*(my+1), hmh);
			int endx = std::min(mapblockw*(mx+1), hmw);

			minh=maxh=heightMap[(endy-1)*hmw+endx-1];

			for (int y=mapblockw*my;y<endy;y++)
				for (int x=mapblockw*mx;x<endx;x++)
				{
					float h = heightMap[hmw * y + x];
					sum += h;

					if (h < minh) minh=h;
					if (h > maxh) maxh=h;
				}

			unsigned char bestm=0;
			int2 best;

			endx = std::min(metalblockw*(mx+1), hmw/2);
			endy = std::min(metalblockw*(my+1), hmh/2);

			for (int y=metalblockw*my;y<endy;y++)
				for (int x=metalblockw*mx;x<endx;x++)
				{
					unsigned char m=metalmap[hmw/2*y+x];
					if (bestm < m) {
						bestm = m;
						best=int2(x,y);
					}
				}

			if (bestm >= minMetalOnSpot)
			{
				spots.push_back (MetalSpot());
				MetalSpot *spot=&spots.back();
				spot->pos = int2 (best.x * 2 + SpotOffset.x, best.y * 2  + SpotOffset.y);
				spot->height = heightMap[spot->pos.y * hmw + spot->pos.x];
			}

			if (bestspot < bestm)
				bestspot = bestm;
		}
	}

	// Calculate metal production constants
	float extractionRange = cb->GetExtractorRadius ();
	float metalFactor = cb->GetMaxMetal ();
	int numspots=0;

	bool *freesq=new bool[hmw*hmh/4];
	std::fill(freesq,freesq+(hmw*hmh/4),true);

	averageProduction = 0.0f;
	for (int a=0;a<spots.size();a++) 
	{
		MetalSpot& spot = spots[a];

		spot.metalProduction = 0.0f;

///		logPrintf ("Metal spot: %dx%d\n", spot.pos.x, spot.pos.y);

		int xBegin = max(0,(int)((spot.pos.x * SQUARE_SIZE - extractionRange) / METAL_MAP_SQUARE_SIZE));
		int xEnd = min(hmw/2-1,(int)((spot.pos.x * SQUARE_SIZE + extractionRange) / METAL_MAP_SQUARE_SIZE));
		int zBegin = max(0,(int)((spot.pos.y * SQUARE_SIZE - extractionRange) / METAL_MAP_SQUARE_SIZE));
		int zEnd = min(hmh/2-1,(int)((spot.pos.y * SQUARE_SIZE + extractionRange) / METAL_MAP_SQUARE_SIZE));

		for (int z=zBegin;z<=zEnd;z++)
			for (int x=xBegin;x<=xEnd;x++)
			{
				int dx=x-spot.pos.x/2;
				int dz=z-spot.pos.y/2;

				if (dx*dx+dz*dz < exRadius * exRadius && freesq[z*hmw/2 + x])
				{
					spot.metalProduction += metalmap [z*hmw/2 + x] * metalFactor;
					freesq[z*hmw/2 + x] = false;
				}
			}

		numspots++;
		averageProduction += spot.metalProduction;
	}

	delete[] freesq;

	if (numspots) {
		averageProduction /= numspots;
		std::vector <MetalSpot> newSpots;

		for (int a=0;a<spots.size();a++)
			if (spots[a].metalProduction > 0.8 * averageProduction)
				newSpots.push_back (spots[a]);
		spots=newSpots;

		AlignSpots(metalmap, hmw,hmh);
	}

	//	for (int a=0;a<spots.size();a++) 
	//		logPrintf ("production(%d)=%f\n", a,spots[a].metalProduction);

	CalcBlocks(cb);

	if (debugShowSpots)
	{
		const UnitDef* mex=cb->GetUnitDef("CORMEX");

		if (mex) for (int a=0;a<spots.size();a++) {
			//logPrintf ("production(%d)=%f\n", a,spots[a].metalProduction);
			float3 pos (spots[a].pos.x * SQUARE_SIZE, 0.0f, spots[a].pos.y * SQUARE_SIZE);

			Calc2DBuildPos (pos, mex);
            pos.y=cb->GetElevation(pos.x, pos.z);
			
			cb->DrawUnit ("CORMEX", pos, 0.0f, 2000000, cb->GetMyAllyTeam(), false, true);
		}
	}

	averageProduction = numspots>0 ? averageProduction/numspots : 0;
	logPrintf ("Blocks: %dx%d, Numspots=%d, AverageProduction=%f\n", width,height, numspots,averageProduction);
}

void MetalSpotMap::AlignSpots(const uchar*metalmap,int mapw,int maph)
{
}

/*


#define METAL_CACHE_EXT "mmapcache"

string MetalSpotMap::GetMapCacheName (const char *mapname)
{
	string s = AI_PATH;
	s += mapname;
	const char *ext = "." METAL_CACHE_EXT;
	s.replace (s.begin() + s.rfind ('.'), s.end(), ext, ext+strlen(ext));
	return s;
}

bool MetalSpotMap::LoadCache (const char *mapname)
{
	string fn = GetMapCacheName (mapname);
	return false;
}

bool MetalSpotMap::SaveCache (const char *mapname)
{
	string fn = GetMapCacheName (mapname);
	FILE *f = fopen (fn.c_str(), "wb");

	if (!f) {
		logPrintf ("Error: Failed to save metal spot cache: %s.\n", fn.c_str());
		return false;
	}

	char t = MMAPCACHE_VERSION;
	fputc (t, f);

	if (ferror(f))
	{
		logPrintf ("Error while writing metal map cache to %s\n", fn.c_str());
		fclose (f);
		remove (fn.c_str());
		return false;
	}

	fclose (f);

	return true;
}

*/

