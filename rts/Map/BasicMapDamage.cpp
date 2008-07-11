#include "StdAfx.h"
#include "BasicMapDamage.h"
#include "ReadMap.h"
#include "MapInfo.h"
#include "BaseGroundDrawer.h"
#include "HeightMapTexture.h"
#include "Rendering/Env/BaseTreeDrawer.h"
#include "TimeProfiler.h"
#include "Sim/Misc/GroundBlockingObjectMap.h"
#include "Sim/Misc/LosHandler.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "LogOutput.h"
#include "Sim/Path/PathManager.h"
#include "Sim/Features/FeatureHandler.h"
#include "Sim/Units/UnitTypes/Building.h"
#include "Sim/Units/UnitDef.h"
#include "mmgr.h"

CBasicMapDamage::CBasicMapDamage(void)
{
	const int numQuads = qf->GetNumQuadsX() * qf->GetNumQuadsZ();
	inRelosQue = SAFE_NEW bool[numQuads];
	for (int a = 0; a < numQuads; ++a) {
		inRelosQue[a]=false;
	}
	relosSize=0;
	neededLosUpdate=0;

	for(int a=0;a<=200;++a){
		float r=a/200.0f;
		float d=cos((r-0.1f)*(PI+0.3f))*(1-r)*(0.5f+0.5f*cos(std::max(0.0f,r*3-2)*PI));
		craterTable[a]=d;
	}
	for(int a=201;a<10000;++a){
		craterTable[a]=0;
	}
	for(int a=0;a<256;++a)
		invHardness[a]=1.0f/mapInfo->terrainTypes[a].hardness;

	mapHardness = mapInfo->map.hardness;

	disabled = false;
}

CBasicMapDamage::~CBasicMapDamage(void)
{
	while(!explosions.empty()){
		delete explosions.front();
		explosions.pop_front();
	}
	delete[] inRelosQue;
}

void CBasicMapDamage::Explosion(const float3& pos, float strength,float radius)
{
	if(pos.x<0 || pos.z<0 || pos.x>gs->mapx*SQUARE_SIZE || pos.z>gs->mapy*SQUARE_SIZE){
//		logOutput.Print("Map damage explosion outside map %.0f %.0f",pos.x,pos.z);
		return;
	}
	if(strength<10 || radius<8)
		return;

	radius*=1.5f;

	Explo* e=SAFE_NEW Explo;
	e->pos=pos;
	e->strength=strength;
	e->ttl=10;
	e->x1=std::max((int)(pos.x-radius)/SQUARE_SIZE,2);
	e->x2=std::min((int)(pos.x+radius)/SQUARE_SIZE,gs->mapx-3);
	e->y1=std::max((int)(pos.z-radius)/SQUARE_SIZE,2);
	e->y2=std::min((int)(pos.z+radius)/SQUARE_SIZE,gs->mapy-3);

	float* heightmap = readmap->GetHeightmap();
	float baseStrength=-pow(strength,0.6f)*3/mapHardness;
	float invRadius=1.0f/radius;
	for(int y=e->y1;y<=e->y2;++y){
		for(int x=e->x1;x<=e->x2;++x){
			CSolidObject* so = groundBlockingObjectMap->GroundBlockedUnsafe(y * gs->mapx + x);
			// don't change squares with building on them here
			if (so && so->blockHeightChanges) {
				e->squares.push_back(0);
				continue;
			}
			float dist=pos.distance2D(float3(x*SQUARE_SIZE,0,y*SQUARE_SIZE));							//calculate the distance
			float relDist=dist*invRadius;																					//normalize distance
			float dif=baseStrength*craterTable[int(relDist*200)]*invHardness[readmap->typemap[(y/2)*gs->hmapx+x/2]];

			float prevDif=heightmap[y*(gs->mapx+1)+x]-readmap->orgheightmap[y*(gs->mapx+1)+x];

//			prevDif+=dif*5;

			if(prevDif*dif>0)
				dif/=fabs(prevDif)*0.1f+1;
			e->squares.push_back(dif);
			if(dif<-0.3f && strength>200)
				treeDrawer->RemoveGrass(x,y);
		}
	}
	std::vector<CUnit*> units=qf->GetUnitsExact(pos,radius);
	for(std::vector<CUnit*>::iterator ui=units.begin();ui!=units.end();++ui){		//calculate how much to offset the buildings in the explosion radius with (while still keeping the ground under them flat
		if((*ui)->blockHeightChanges && (*ui)->isMarkedOnBlockingMap){
			CUnit* unit=*ui;

			float totalDif=0;
			for(int z=unit->mapPos.y; z<unit->mapPos.y+unit->ysize; z++){
				for(int x=unit->mapPos.x; x<unit->mapPos.x+unit->xsize; x++){
					float dist=pos.distance2D(float3(x*SQUARE_SIZE,0,z*SQUARE_SIZE));							//calculate the distance
					float relDist=dist*invRadius;																					//normalize distance
					float dif=baseStrength*craterTable[int(relDist*200)]*invHardness[readmap->typemap[(z/2)*gs->hmapx+x/2]];
					float prevDif=heightmap[z*(gs->mapx+1)+x]-readmap->orgheightmap[z*(gs->mapx+1)+x];

					if(prevDif*dif>0)
						dif/=fabs(prevDif)*0.1f+1;
					totalDif+=dif;
				}
			}
			totalDif/=unit->xsize*unit->ysize;
			if(totalDif!=0){
				ExploBuilding eb;
				eb.id=(*ui)->id;
				eb.dif=totalDif;
				eb.tx1=unit->mapPos.x;
				eb.tx2=unit->mapPos.x+unit->xsize;
				eb.tz1=unit->mapPos.y;
				eb.tz2=unit->mapPos.y+unit->ysize;
				e->buildings.push_back(eb);
			}
		}
	}

	explosions.push_back(e);

	readmap->Explosion(pos.x,pos.z,strength);
}

void CBasicMapDamage::RecalcArea(int x1, int x2, int y1, int y2)
{
	float* heightmap = readmap->GetHeightmap();
	float* centerheightmap = readmap->centerheightmap;
	for (int y = y1; y < y2; y++) {
		for (int x = x1; x < x2; x++) {
			const float height = heightmap[((y)     * (gs->mapx + 1)) + (x)    ]
			                   + heightmap[((y)     * (gs->mapx + 1)) + (x + 1)]
			                   + heightmap[((y + 1) * (gs->mapx + 1)) + (x)    ]
			                   + heightmap[((y + 1) * (gs->mapx + 1)) + (x + 1)];
			centerheightmap[(y * gs->mapx) + x] = height * 0.25f;
		}
	}

	/*int hy2=min(gs->hmapy-1,y2/2);
	int hx2=min(gs->hmapx-1,x2/2);
	for(int y=y1/2;y<=hy2;y++)
		for(int x=x1/2;x<=hx2;x++)
			readmap->mipHeightmap[1][y*gs->hmapx+x]=heightmap[(y*2+1)*(gs->mapx+1)+(x*2+1)];*/

	float numHeightMipMaps = readmap->numHeightMipMaps - 1;
	float** mipHeightmap = readmap->mipHeightmap;
	for (int i = 0; i < numHeightMipMaps; i++) {
		int hmapx = gs->mapx >> i;
		for (int y = ((y1 >> i) & (~1)); y < (y2 >> i); y += 2) {
			for (int x = ((x1 >> i) & (~1)); x < (x2 >> i); x += 2) {
				float height = mipHeightmap[i][(x)+(y)*hmapx];
				height += mipHeightmap[i][(x)  +(y+1)*hmapx];
				height += mipHeightmap[i][(x+1)+(y)  *hmapx];
				height += mipHeightmap[i][(x+1)+(y+1)*hmapx];
				mipHeightmap[i+1][(x/2)+(y/2)*hmapx/2] = height * 0.25f;
			}
		}
	}

	float3 n1,n2,n3,n4;
	int decy=std::max(0,y1-1);
	int incy=std::min(gs->mapy-1,y2+1);
	int decx=std::max(0,x1-1);
	int incx=std::min(gs->mapx-1,x2+1);

	float3* facenormals = readmap->facenormals;
	for (int y = decy; y <= incy; y++) {
		for (int x = decx; x <= incx; x++) {
			float3 e1(-SQUARE_SIZE,heightmap[y*(gs->mapx+1)+x]-heightmap[y*(gs->mapx+1)+x+1],0);
			float3 e2( 0,heightmap[y*(gs->mapx+1)+x]-heightmap[(y+1)*(gs->mapx+1)+x],-SQUARE_SIZE);

			float3 n=e2.cross(e1);
			n.Normalize();

			facenormals[(y*gs->mapx+x)*2] = n;

			e1 = float3( SQUARE_SIZE,heightmap[(y+1)*(gs->mapx+1)+x+1]-heightmap[(y+1)*(gs->mapx+1)+x],0);
			e2 = float3( 0,heightmap[(y+1)*(gs->mapx+1)+x+1]-heightmap[(y)*(gs->mapx+1)+x+1],SQUARE_SIZE);

			n = e2.cross(e1);
			n.Normalize();

			facenormals[(y*gs->mapx+x)*2+1] = n;
		}
	}

	for(int y = std::max(2,(y1&0xfffffe)); y <= std::min(gs->mapy-3,y2); y += 2) {
		for(int x = std::max(2,(x1&0xfffffe)); x <= std::min(gs->mapx-3,x2); x += 2) {
			float3 e1(-SQUARE_SIZE*4,heightmap[(y-1)*(gs->mapx+1)+x-1]-heightmap[(y-1)*(gs->mapx+1)+x+3],0);
			float3 e2( 0,heightmap[(y-1)*(gs->mapx+1)+x-1]-heightmap[(y+3)*(gs->mapx+1)+x-1],-SQUARE_SIZE*4);

			float3 n = e2.cross(e1);
			n.Normalize();

			e1 = float3( SQUARE_SIZE*4,heightmap[(y+3)*(gs->mapx+1)+x+3]-heightmap[(y+3)*(gs->mapx+1)+x-1],0);
			e2 = float3( 0,heightmap[(y+3)*(gs->mapx+1)+x+3]-heightmap[(y-1)*(gs->mapx+1)+x+3],SQUARE_SIZE*4);

			float3 n2 = e2.cross(e1);
			n2.Normalize();

			readmap->slopemap[(y/2)*gs->hmapx+(x/2)] = 1-(n.y+n2.y)*0.5f;
		}
	}

	pathManager->TerrainChange(x1, y1, x2, y2);
	featureHandler->TerrainChanged(x1, y1, x2, y2);
	readmap->HeightmapUpdated(x1, x2, y1, y2);

	decy = std::max(0,(y1*SQUARE_SIZE-QUAD_SIZE/2)/QUAD_SIZE);
	incy = std::min(qf->GetNumQuadsZ()-1,(y2*SQUARE_SIZE+QUAD_SIZE/2)/QUAD_SIZE);
	decx = std::max(0,(x1*SQUARE_SIZE-QUAD_SIZE/2)/QUAD_SIZE);
	incx = std::min(qf->GetNumQuadsX()-1,(x2*SQUARE_SIZE+QUAD_SIZE/2)/QUAD_SIZE);

	for (int y = decy; y <= incy; y++) {
		for (int x = decx; x <= incx; x++) {
			if (inRelosQue[y*qf->GetNumQuadsX()+x]) {
				continue;
			}
			RelosSquare rs;
			rs.x = x;
			rs.y = y;
			rs.neededUpdate = gs->frameNum;
			rs.numUnits = qf->GetQuadAt(x, y).units.size();
			relosSize += rs.numUnits;
			inRelosQue[y*qf->GetNumQuadsX()+x] = true;
			relosQue.push_back(rs);
		}
	}
	heightMapTexture.UpdateArea(x1, y1, x2, y2);
}


void CBasicMapDamage::Update(void)
{
	SCOPED_TIMER("Map damage");

	std::deque<Explo*>::iterator ei;
	float* heightmap=readmap->GetHeightmap();

	for(ei=explosions.begin();ei!=explosions.end();++ei){
		Explo* e=*ei;
		if (e->ttl<0) continue;
		--e->ttl;

		int x1=e->x1;
		int x2=e->x2;
		int y1=e->y1;
		int y2=e->y2;
		std::vector<float>::iterator si=e->squares.begin();
		for(int y=y1;y<=y2;++y){
			for(int x=x1;x<=x2;++x){
				float dif=*(si++);
				heightmap[y*(gs->mapx+1)+x]+=dif;
			}
		}
		for(std::vector<ExploBuilding>::iterator bi=e->buildings.begin();bi!=e->buildings.end();++bi){
			float dif=bi->dif;
			int tx1 = bi->tx1;
			int tx2 = bi->tx2;
			int tz1 = bi->tz1;
			int tz2 = bi->tz2;

			for(int z=tz1; z<tz2; z++){
				for(int x=tx1; x<tx2; x++){
					heightmap[z*(gs->mapx+1)+x]+=dif;
				}
			}
			CUnit* unit=uh->units[bi->id];
			if(unit){
				unit->pos.y+=dif;
				unit->midPos.y+=dif;
			}
		}
		readmap->ExplosionUpdate(e->x1,e->x2,e->y1,e->y2);
		if(e->ttl==0){
			float3 pos=e->pos;
			RecalcArea(x1-2,x2+2,y1-2,y2+2);
	/*		for(int y=y1>>3;y<=y2>>3;++y){
				for(int x=x1>>3;x<=x2>>3;++x){
					if(!inRejuvQue[y][x]){
						RejuvSquare rs;
						rs.x=x;
						rs.y=y;
						rejuvQue.push_back(rs);
						inRejuvQue[y][x]=true;
					}
				}
			}*/
		}
	}
	while(!explosions.empty() && explosions.front()->ttl==0){
		delete explosions.front();
		explosions.pop_front();
	}

/*
	nextRejuv+=rejuvQue.size()/300.0f;
	while(nextRejuv>0){
		nextRejuv-=1;
		int bx=rejuvQue.front().x;
		int by=rejuvQue.front().y;
		bool damaged=false;

		for(int y=by*16;y<by*16+16;++y){
			for(int x=bx*16;x<bx*16+16;++x){
				if(readmap->damagemap[y*1024+x]!=0){
					damaged=true;
					if(readmap->damagemap[y*1024+x]>5){
						int hsquare=((y+1)/2)*(gs->mapx+1)+(x+1)/2;
						readmap->heightmap[hsquare]-=(readmap->heightmap[hsquare]-readmap->orgheightmap[hsquare])*0.003f;
						readmap->damagemap[y*1024+x]-=4;
					} else {
						readmap->damagemap[y*1024+x]=0;
						if(readmap->typemap[(y/2)*512+x/2]&128){
							readmap->typemap[(y/2)*512+x/2]&=127;
							treeDrawer->ResetPos(float3(x*4,0,y*4));
						}
					}
				}
			}
		}
		if(damaged){
			rejuvQue.push_back(rejuvQue.front());
			RecalcArea(bx*8,bx*8+8,by*8,by*8+8);
			readmap->RecalcTexture(bx*8,bx*8+8,by*8,by*8+8);
		} else {
			inRejuvQue[by][bx]=false;
		}
		rejuvQue.pop_front();
	}*/
	UpdateLos();
}

void CBasicMapDamage::UpdateLos(void)
{
	int updateSpeed=(int)(relosSize*0.01f)+1;

	if(relosUnits.empty()){
		if(relosQue.empty())
			return;
		RelosSquare* rs=&relosQue.front();

		const std::list<CUnit*>& units = qf->GetQuadAt(rs->x, rs->y).units;
		for (std::list<CUnit*>::const_iterator ui = units.begin(); ui != units.end(); ++ui) {
			relosUnits.push_back((*ui)->id);
		}
		relosSize-=rs->numUnits;
		neededLosUpdate=rs->neededUpdate;
		inRelosQue[rs->y*qf->GetNumQuadsX()+rs->x]=false;
		relosQue.pop_front();
	}
	for(int a=0;a<updateSpeed;++a){
		if(relosUnits.empty())
			return;
		CUnit* u=uh->units[relosUnits.front()];
		relosUnits.pop_front();

		if(u==0 || u->lastLosUpdate>=neededLosUpdate){
			continue;
		}
		loshandler->MoveUnit(u,true);
	}
}
