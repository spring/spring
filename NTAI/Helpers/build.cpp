#include "../helper.h"

#define FACTORY_SPACE	4 // factory space is in buildmap squares, which have the size of LLTs

#define BM_SQUARE_SIZE (SQUARE_SIZE*4)

#define BM_FREE 0
#define BM_TAKEN		1
#define BM_GEOTHERMAL	2
#define BM_MEXPOS		4
const struct nOffset_t { int x,y; } static nOffsetTbl[8] = {
	{ -1,-1 }, { 0, -1 }, { 1, -1 }, { -1, 0 }, { 1, 0 }, { -1, 1 }, { 0, 1 }, { 1, 1 }
};

int numSearchOffsets;
static SearchOffset *searchOffsetTable = 0;

bool SearchOffsetComparator (const SearchOffset& a, const SearchOffset& b){
	return a.qdist < b.qdist;
}

SearchOffset* GetSearchOffsetTable (){
	const int w=256;
	if (!searchOffsetTable) {
		SearchOffset *tmp = new SearchOffset [w*w];
		for (int y=0;y<w;y++)
			for (int x=0;x<w;x++)	{
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
void FreeSearchOffsetTable (){
	if (searchOffsetTable) delete[] searchOffsetTable;
}

BuildPlacer::BuildPlacer(Global* GLI){
	G = GLI;
	w=h=0;
	//bmap=0;
}
BuildPlacer::~BuildPlacer(){
	/*if(bmap) {
		delete[] bmap;
		bmap=0;
	}*/
	FreeSearchOffsetTable();
}

bool BuildPlacer::Init(){
	G->L->print("BuildPlacer::Init");
	w = G->cb->GetMapWidth() / 4; // the xta LLT is 4x4, so this way it will cover exactly one pixel
	h = G->cb->GetMapHeight() / 4;
	//G->L->print("BuildPlacer::Init1");
	//bmap = new uchar[w*h];
	//memset(bmap,0,w*h);
	vector<float3>* metalmap = G->M->getHotSpots();
	//G->L->print("BuildPlacer::Init2");
	for (vector<float3>::const_iterator a = metalmap->begin() ;a!=metalmap->end();a++){
		//G->L->print("BuildPlacer::Init2a");
		float3 pos = *a;
		pos.x /= 4; pos.z /= 4;
		//G->L->print("BuildPlacer::Init2b");
		int hg = pos.z*w+pos.x;
		//G->L->print("BuildPlacer::Init2b2");
		bmap[hg] = BM_MEXPOS;
		//G->L->print("BuildPlacer::Init2c");
		/*if(pos.x < 0){
			hg = (pos.z+1)*w+pos.x;
			bmap[hg] = BM_MEXPOS;
		}
		//G->L->print("BuildPlacer::Init2d");
		bmap[hg+1] = BM_MEXPOS;
		//G->L->print("BuildPlacer::Init2e");
		hg = (pos.z+1)*w+pos.x+1;
		if(pos.y > 0) bmap[hg] = BM_MEXPOS;
		//G->L->print("BuildPlacer::Init2f");*/
	}
	G->L->print("BuildPlacer::Init4");
	this->WriteTGA("Buildmap.tga");
	return true;
}
int BuildPlacer::GetUnitSpace (const UnitDef *def){
	//if (def->builder)
		return 3;
	//return 0;
	//return !def->weapons.empty() ? 2 : 0;
}

void BuildPlacer::Calc2DBuildPos(float3& pos, const UnitDef* ud){
	if(ud->xsize&2)
		pos.x=floor((pos.x)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.x=floor((pos.x+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;
	if(ud->ysize&2)
		pos.z=floor((pos.z)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.z=floor((pos.z+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;
}

void BuildPlacer::Mark (const UnitDef* def, const float3& opos, bool occupy){
	G->L->print("BuildPlacer::Mark");
	float3 pos = opos;
	Calc2DBuildPos (pos, def);
	string s = "Unit";
	s += def->name.c_str();
	s += "marked on buildmap";
	G->L->print(s);
	int space = GetUnitSpace (def);
	int xstart = max(0, (int(pos.x)/SQUARE_SIZE - def->xsize/2) / 4 - space);
	int ystart = max(0, (int(pos.z)/SQUARE_SIZE - def->ysize/2) / 4 - space);
	int xend = min (w, (int(pos.x)/SQUARE_SIZE + def->xsize/2) / 4 + space);
	int yend = min (h, (int(pos.z)/SQUARE_SIZE + def->ysize/2) / 4 + space);
	for (int y=ystart;y<yend;y++)
		for (int x=xstart;x<xend;x++){
			if (occupy)
				Get(x,y) |= BM_TAKEN;
			else
				Get(x,y) &= ~BM_TAKEN;
		}
		this->WriteTGA("Buildmap.tga");
}
/*int2 InfoMap::FindSafeBuildingSector (float3 sp, int maxdist, int reqspace)
{
	int qd = maxdist*maxdist;
	SearchOffset *tbl = GetSearchOffsetTable ();
	int2 sector = GetMapInfoCoords (sp);

	int bestscore;
	int2 bestsector (-1,0);

	for (int a=0;a<numSearchOffsets;a++)	{
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
		int score = sqrtf (ofs.qdist) * blk.heightDif * gi.threat;

		int dx=baseCenter.x/mblocksize-x, dy=baseCenter.y/mblocksize-y;
		score *= max(1.0f, sqrtf (dx*dx+dy*dy));// + (baseDirection.x * ofs.dx + baseDirection.y * ofs.dy));
		if (bestsector.x < 0 || bestscore > score){
			bestscore = score;
			bestsector = int2 (x,y);
		}
	}
	MapInfo *b=GetMapInfo(bestsector);
//	logPrintf ("RequestSafeBuildingZone(): heightdif=%f\n", b->heightDif);
	return bestsector;
}*/
bool BuildPlacer::FindBuildPosition (const UnitDef *def, const float3& startPos, float3& out){
	G->L->print("BuildPlacer::FindBuildPosition");
	SearchOffset* so = GetSearchOffsetTable();
	float3 p = startPos;
	Calc2DBuildPos(p, def);
	float3 start = float3(int(p.x) / BM_SQUARE_SIZE,0, int(p.z) / BM_SQUARE_SIZE);
	int space = GetUnitSpace(def);
	vector<float3> positions;
	for (int a=0;a<numSearchOffsets;a++, so++){
		int x,y;
		x = start.x + so->dx;
		y = start.y + so->dy;
		if (x < 0 || y < 0 || x >= w || y >= h)
			continue;
		float3 pos(x, 0, y);
		pos *= BM_SQUARE_SIZE;
		if (G->cb->CanBuildAt(def, pos)){
			int xstart = int(x*4 - def->xsize/2) / 4 - space;
			int ystart = int(y*4 - def->ysize/2) / 4 - space;
			int xend = int(x*4 + def->xsize/2) / 4 + space;
			int yend = int(y*4 + def->ysize/2) / 4 + space;
			if (xstart < 0 || ystart < 0 || xend >= w || yend >= h) continue;
			bool good=true;
			bool hasGeo=false;
			bool hasMexSpot=false;
			for (int y=ystart;y<yend;y++)
				for (int x=xstart;x<xend;x++){
					int v = Get(x,y);
					if (v & BM_GEOTHERMAL)	hasGeo=true;
					if (v & BM_MEXPOS) hasMexSpot=true;
					if (Get(x,y) & BM_TAKEN)	{
						good=false;
						break;
					}
				}
			if (def->extractsMetal == 0 && hasMexSpot && good){
				pos.y = G->cb->GetElevation (pos.x,pos.z);
				positions.push_back(pos);
				//distances[pos] = pos.distance(p);
				//return true;
			}else 	if (good && (def->needGeo ^ !hasGeo)){
				pos.y = G->cb->GetElevation (pos.x,pos.z);
				positions.push_back(pos);
				//return true;
			} else if(good){
				pos.y = G->cb->GetElevation (pos.x,pos.z);
				positions.push_back(pos);
			}
		}
	}
	// find nearest position in the positions and distances arrays.
	float3 bestpos(0,0,0);
	float bestdist=20000;
	if(positions.empty()){
		G->L->print("positions vector<float3> empty?!?!?!");
		out = bestpos;
		return false;
	}
	for(vector<float3>::iterator pq = positions.begin();pq !=positions.end();++pq){
		float ndist = pq->distance(p);
		if(ndist<bestdist){
			bestdist = ndist;
			bestpos = *pq;
		}
	}
	if(bestpos.x = 0){
		G->L->print("bestpos.x = 0!");
		out = bestpos;
		return false;
	}
	if(bestpos == p) G->L->print("bestpos == p");
	out = bestpos;
	return true;
}

bool BuildPlacer::WriteTGA (const char *file){
	// open file
	FILE *fp=fopen(file, "wb");
	if(!fp)	return false;
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
	int p =0;
	for (int y=0;y<h;y++)
		for (int x=0;x<w;x++)	{
			const int div=4;
			out[0] = (bmap[p] & BM_TAKEN) ? 255 : 0;
			out[1] = (bmap[p] & BM_GEOTHERMAL) ? 255 : 0;
			out[2] = (bmap[p] & BM_MEXPOS) ? 255 : 0;

			fwrite (out, 3, 1, fp);
			p++;
		}
	fclose(fp);
	return true;
}
float3 BuildPlacer::Pos2BuildPos(float3 pos, const UnitDef* ud){
	if(ud->xsize&2)
		pos.x=floor((pos.x)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.x=floor((pos.x+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;
	if(ud->ysize&2)
		pos.z=floor((pos.z)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.z=floor((pos.z+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;
	pos.y=G->cb->GetElevation(pos.x,pos.z);
	if(ud->floater && pos.y<0)
		pos.y = -ud->waterline;
	return pos;
}
float3 BuildPlacer::ClosestBuildSite(const UnitDef* unitdef,float3 pos,float searchRadius,int minDist){
	//todo fix so you cant detect enemy buildings with it
	float bestDist=searchRadius;
	float3 bestPos(-1,0,0);

	//CFeature* feature;
	int allyteam= G->cb->GetMyAllyTeam();
	for(float z=pos.z-searchRadius;z<pos.z+searchRadius;z+=SQUARE_SIZE*2){
		for(float x=pos.x-searchRadius;x<pos.x+searchRadius;x+=SQUARE_SIZE*2){
			float3 p(x,0,z);
			p = this->Pos2BuildPos (p, unitdef);
			float dist=pos.distance2D(p);
			if(dist<bestDist && G->cb->CanBuildAt(unitdef,p)/* && (!feature || feature->allyteam!=allyteam)*/){
				int xs=x/SQUARE_SIZE;
				int zs=z/SQUARE_SIZE;
				bool good=true;
				//for(int z2=max(0,zs-unitdef->ysize/2-minDist);z2<min(G->cb->GetMapHeight(),zs+unitdef->ysize+minDist);++z2){
				//	for(int x2=max(0,xs-unitdef->xsize/2-minDist);x2<min(G->cb->GetMapWidth(),xs+unitdef->xsize+minDist);++x2){
						//CSolidObject* so=readmap->groundBlockingObjectMap[z2*gs->mapx+x2];
					int gh[10000];
					int unum;
					unum = G->cb->GetFriendlyUnits(gh,p,minDist+unitdef->xsize);
					for(int ghy = 0; ghy <unum;ghy++){
						if(G->cb->GetUnitSpeed(gh[ghy])==0){
							good=false;
							break;
						}
					}
				//}
				if(good){
					bestDist=dist;
					bestPos=p;
				}
			}
		}
	}
	return bestPos;	
}