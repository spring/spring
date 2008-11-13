//-------------------------------------------------------------------------
// JCAI version 0.21
//
// A skirmish AI for the TA Spring engine.
// Copyright Jelmer Cnossen
// 
// Released under GPL license: see LICENSE.html for more information.
//-------------------------------------------------------------------------
/* The buildmap idea is based on Alik's MapGridUtils */
#include "BaseAIDef.h"
#include "BuildMap.h"
#include "InfoMap.h"
#include "BuildTable.h"
#include "MetalSpotMap.h"

#define FACTORY_SPACE	4 // factory space is in buildmap squares, which have the size of LLTs

#define BM_SQUARE_SIZE (SQUARE_SIZE*4)

#define BM_TAKEN		1
#define BM_GEOTHERMAL	2
#define BM_MEXPOS		4

BuildMap::BuildMap ()
{
	w=h=0;
	map=0;

	metalmap=0;
	callback=0;
}

BuildMap::~BuildMap()
{
	if(map) {
		delete[] map;
		map=0;
	}
}

int BuildMap::GetUnitSpace (const UnitDef *def)
{
	BuildTable::UDef *cd=buildTable.GetCachedDef (def->id);
	if (cd->IsBuilder())
		return 3;

	return cd->weaponDamage ? 2 : 0;
}

bool BuildMap::Init (IAICallback *cb, MetalSpotMap *msm)
{
	w = cb->GetMapWidth() / 4; // the xta LLT is 4x4, so this way it will cover exactly one pixel
	h = cb->GetMapHeight() / 4;

	map = new uchar[w*h];
	memset (map,0,w*h);

	metalmap = msm;
	callback = cb; 

	for (int a=0;a<msm->GetNumSpots();a++)
	{
		MetalSpot *spot = msm->GetSpot (a);

		int2 pos = spot->pos;
		pos.x /= 4; pos.y /= 4;

		map [pos.y*w+pos.x] |= BM_MEXPOS;
		if(pos.x < 0) map [(pos.y+1)*w+pos.x] |= BM_MEXPOS;
		map [pos.y*w+pos.x+1] |= BM_MEXPOS;
		if(pos.y < 0) map [(pos.y+1)*w+pos.x+1] |= BM_MEXPOS;
	}

	return true;
}

void BuildMap::Calc2DBuildPos(float3& pos, const UnitDef* ud)
{
	if(ud->xsize&2)
		pos.x=floor((pos.x)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.x=floor((pos.x+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;

	if(ud->zsize&2)
		pos.z=floor((pos.z)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.z=floor((pos.z+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;
}

void BuildMap::Mark (const UnitDef* def, const float3& opos, bool occupy)
{
	float3 pos = opos;
	Calc2DBuildPos (pos, def);

	logPrintf ("Unit %s marked on buildmap.\n", def->name.c_str());

	BuildTable::UDef *cd = buildTable.GetCachedDef (def->id);

	int space = GetUnitSpace (def);

	int xstart = max (0, (int(pos.x)/SQUARE_SIZE - def->xsize/2) / 4 - space);
	int ystart = max (0, (int(pos.z)/SQUARE_SIZE - def->zsize/2) / 4 - space);
	int xend = min (w, (int(pos.x)/SQUARE_SIZE + def->xsize/2) / 4 + space);
	int yend = min (h, (int(pos.z)/SQUARE_SIZE + def->zsize/2) / 4 + space);

	for (int y=ystart;y<yend;y++)
		for (int x=xstart;x<xend;x++)
		{
			if (occupy)
				Get(x,y) |= BM_TAKEN;
			else
				Get(x,y) &= ~BM_TAKEN;
		}
}

bool BuildMap::FindBuildPosition (const UnitDef *def, const float3& startPos, float3& out)
{
	SearchOffset* so = GetSearchOffsetTable();
	float3 p = startPos;
	Calc2DBuildPos (p, def);
	int2 start = int2(int(p.x) / BM_SQUARE_SIZE, int(p.z) / BM_SQUARE_SIZE);

	BuildTable::UDef *cd = buildTable.GetCachedDef (def->id);

	int space = GetUnitSpace (def);

	for (int a=0;a<numSearchOffsets;a++, so++)
	{
		int x,y;

		x = start.x + so->dx;
		y = start.y + so->dy;

		if (x < 0 || y < 0 || x >= w || y >= h)
			continue;

		float3 pos(x, 0, y);
		pos *= BM_SQUARE_SIZE;

		if (callback->CanBuildAt (def, pos))
		{
			int xstart = int(x*4 - def->xsize/2) / 4 - space;
			int ystart = int(y*4 - def->zsize/2) / 4 - space;
			int xend = int(x*4 + def->xsize/2) / 4 + space;
			int yend = int(y*4 + def->zsize/2) / 4 + space;

			if (xstart < 0 || ystart < 0 || xend >= w || yend >= h)
				continue;

			bool good=true;
			bool hasGeo=false;
			bool hasMexSpot=false;
			for (int y=ystart;y<yend;y++)
				for (int x=xstart;x<xend;x++)
				{
					uchar v = Get(x,y);
					if (v & BM_GEOTHERMAL)
						hasGeo=true;
					if (v & BM_MEXPOS)
						hasMexSpot=true;

					if (Get(x,y) & BM_TAKEN)
					{
						good=false;
						break;
					}
				}

			if (def->extractsMetal == 0 && hasMexSpot)
				continue;

			if (good && (def->needGeo ^ !hasGeo))
			{
				pos.y = callback->GetElevation (pos.x,pos.z);
				out = pos;
				return true;
			}
		}
	}

	return false;
}


/*
Saves a 24 bit uncompressed TGA image
*/
bool BuildMap::WriteTGA (const char *file)
{
	// open file
	FILE *fp=fopen(file, "wb");
	if(!fp)
		return false;

	// fill & write header
	char targaheader[18];
	memset(targaheader, 0, sizeof(targaheader));

	targaheader[2] = 2;		/* image type = uncompressed gray-scale */
	targaheader[12] = (char) (w & 0xFF);
	targaheader[13] = (char) (w >> 8);
	targaheader[14] = (char) (h & 0xFF);
	targaheader[15] = (char) (h >> 8);
	targaheader[16] = 24; /* 24 bit color */
	targaheader[17] = 0x20;	/* Top-down, non-interlaced */

	fwrite(targaheader, 18, 1, fp);

	// write file
	uchar out[3];

	uchar *p = map;
	for (int y=0;y<h;y++)
		for (int x=0;x<w;x++)
		{
			const int div=4;
			out[0] = (*p & BM_TAKEN) ? 255 : 0;
			out[1] = (*p & BM_GEOTHERMAL) ? 255 : 0;
			out[2] = (*p & BM_MEXPOS) ? 255 : 0;

			fwrite (out, 3, 1, fp);
			p++;
		}
	fclose(fp);

	return true;
}

