// Map
#include "../../Core/helper.h"

CMap::CMap(Global* GL){
	G = GL;
}

float3 CMap::distfrom(float3 Start, float3 Target, float distance){
	NLOG("CMap::distfrom");
	if(CheckFloat3(Start)==false) return UpVector;
	if(CheckFloat3(Target) == false) return UpVector;
	float p = distance/Start.distance2D(Target);
	if(p < 0) p *= -1;
	float dx = Start.x-Target.x;
	if(dx < 0) dx *= -1;
	float dz = Start.z-Target.z;
	if(dz < 0) dz *= -1;
	dz *= p;
	dx *= p;
	float x = Target.x;
	if(Start.x > Target.x){
		x += dx;
	} else{
		x -= dx;
	}
	float z = Target.z;
	if(Start.z > Target.z){
		z += dz;
	}else{
		z -= dz;
	}
	return float3(x,0,z);
	//	float c = Target.distance(Start);
	//	return float3(Target.x + ((distance*(Start.z - Target.z))/c),0,Target.z + ((distance*(Start.x - Target.x))/c));
}

t_direction CMap::WhichCorner(float3 pos){
	NLOG("CMap::WhichCorner");
	// 1= NW, 2=NE,  3= SW, 4=SE
	if ((pos.x<(G->cb->GetMapWidth()*4))&&(pos.z<(G->cb->GetMapHeight()*4))) return t_NW;
	if ((pos.x>(G->cb->GetMapWidth()*4))&&(pos.z<(G->cb->GetMapHeight()*4))) return t_NE;
	if ((pos.x<(G->cb->GetMapWidth()*4))&&(pos.z>(G->cb->GetMapHeight()*4))) return t_SW;
	if ((pos.x>(G->cb->GetMapWidth()*4))&&(pos.z>(G->cb->GetMapHeight()*4))) return t_SE;
	return t_NA;
}

/* returns the base position nearest to the given float3*/
float3 CMap::nbasepos(float3 pos){
	NLOG("CMap::nbasepos");
	if(base_positions.empty() == false){
		float3 best;
		float best_score = 20000000;
		for(vector<float3>::iterator i = base_positions.begin(); i != base_positions.end(); ++i){
			if( i->distance2D(pos) < best_score){
				best = *i;
				best_score = i->distance2D(pos);
			}
		}
		return best;
	}else{
		return basepos;
	}
}

vector<float3> CMap::GetSurroundings(float3 pos){
	NLOG("CMap::GetSurroundings");
	vector<float3> s;
	float3 ss = WhichSector(pos);
	s.push_back(ss);
	ss.z = ss.z - 1;
	s.push_back(ss);
	ss.z = ss.z +2;
	s.push_back(ss);
	ss.z = ss.z -1;
	ss.x = ss.x - 1;
	s.push_back(ss);
	ss.z = ss.z - 1;
	s.push_back(ss);
	ss.z = ss.z +2;
	s.push_back(ss);
	ss.z = ss.z - 1;
	ss.x = ss.x + 2;
	s.push_back(ss);
	ss.z = ss.z - 1;
	s.push_back(ss);
	ss.z = ss.z +2;
	s.push_back(ss);
	return s;
}

// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

float3 CMap::WhichSector(float3 pos){
	NLOG("CMap::WhichSector");
	if(CheckFloat3(pos) == false) return UpVector;
	float3 spos;
	spos.x = pos.x/G->Ch->xSize;
	spos.y = pos.y;
	spos.z = pos.z/G->Ch->ySize;
	if((spos.z == 0)||(spos.z == 1)){
		if((spos.x == 0)||(spos.x == 1)){
			G->L << " strange location x: " << spos.x << "  y: " << spos.y << "  z: "<<spos.z << " from location x: " << pos.x << "  y: " << pos.y << "  z: "<<pos.z << endline;
		}
	}
	return spos;
}

float3 CMap::Rotate(float3 pos, float angle, float3 origin){
	NLOG("CMap::Rotate");
	float3 newpos;
	newpos.x = (pos.x-origin.x)*cos(angle) - (pos.z-origin.z)*sin(angle);
	newpos.z = (pos.x-origin.x)*sin(angle) + (pos.z-origin.z)*cos(angle);
	newpos.x += origin.x;
	newpos.z += origin.z;
	newpos.y = pos.y;
	return newpos;
}

bool CMap::Overlap(float x1, float y1, float w1, float h1, float x2, float y2, float w2, float h2){
	NLOG("CMap::Overlap");
	// A below B
	if(y1 + h1 <= y2)
		return false;
	// B below A
	if(y2 + h2 <= y1)
		return false;
	// A left of B
	if(x1 + w1 <= x2)
		return false;
	// B left of A
	if(x2 + w2 <= x1)
		return false;
	return true;
}

float CMap::GetBuildHeight(float3 pos, const UnitDef* unitdef){
	NLOG("CMap::GetBuildHeight");
	float minh=-5000;
	float maxh=5000;
	int numBorder=0;
	float borderh=0;

	float maxDif=unitdef->maxHeightDif;
	int x1 = (int)max(0.f,(pos.x-(unitdef->xsize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
	int x2 = min(G->cb->GetMapWidth()*8,x1+unitdef->xsize);
	int z1 = (int)max(0.f,(pos.z-(unitdef->ysize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
	int z2 = min(G->cb->GetMapHeight()*8,z1+unitdef->ysize);

	if (x1 > G->cb->GetMapWidth()*8) x1 = G->cb->GetMapWidth()*8;
	if (x2 < 0) x2 = 0;
	if (z1 > G->cb->GetMapHeight()*8) z1 = G->cb->GetMapHeight()*8;
	if (z2 < 0) z2 = 0; 

	const float* heightmap = G->cb->GetHeightMap();
	for(int x=x1; x<=x2; x++){
		for(int z=z1; z<=z2; z++){
			float orgh=heightmap[z*(G->cb->GetMapWidth()*8+1)+x];
			float h=heightmap[z*(G->cb->GetMapWidth()*8+1)+x];
			if(x==x1 || x==x2 || z==z1 || z==z2){
				numBorder++;
				borderh+=h;
			}
			if(minh<min(h,orgh)-maxDif)
				minh=min(h,orgh)-maxDif;
			if(maxh>max(h,orgh)+maxDif)
				maxh=max(h,orgh)+maxDif;
		}
	}
	float h=borderh/numBorder;

	if(h<minh && minh<maxh)
		h=minh+0.01f;
	if(h>maxh && maxh>minh)
		h=maxh-0.01f;

	return h;
}
void CMap::GenerateTerrainTypes(){
	yMapSize=(int)G->cb->GetMapHeight()/16;
	xMapSize=(int)G->cb->GetMapWidth()/16;
	float3 df = float3(8,0,8);
	float3 du = float3(16,0,16);
	float3 pos = float3(0,0,0);
	for(int y = 0; y < yMapSize;y++){
		for(int x = 0; x < xMapSize;x++){
			//
			CSector c;
			c.ucorner = pos;
			c.lcorner = pos + du;
			c.centre = pos + df;
			c.csect = float3((float)x,0,(float)y);
			c.slope = (c.ucorner.x-c.lcorner.x)/(c.ucorner.y-c.lcorner.y);
			if(c.slope < 0){
				c.slope *= -1;
			}
			float3 tpos;
			buildmap[x][y] = c;
			pos.x += 16;

		}
		pos.y += 16;
	}
	//for(int i)
}

float3 CMap::Pos2BuildPos(float3 pos, const UnitDef* ud){
	NLOG("CMap::Pos2BuildPos");
	if(ud->xsize&2)
		pos.x=floor((pos.x)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.x=floor((pos.x+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;

	if(ud->ysize&2)
		pos.z=floor((pos.z)/(SQUARE_SIZE*2))*SQUARE_SIZE*2+8;
	else
		pos.z=floor((pos.z+8)/(SQUARE_SIZE*2))*SQUARE_SIZE*2;

	pos.y=GetBuildHeight(pos,ud);
	if(ud->floater && pos.y<0)
		pos.y = -ud->waterline;

	return pos;
}

bool SearchOffsetComparator (const SearchOffset& a, const SearchOffset& b){
	return a.qdist < b.qdist;
}

vector<SearchOffset> CMap::GetSearchOffsetTable(int radius){
	NLOG("CMap::GetSearchOffsetTable");
	vector <SearchOffset> searchOffsets;
	int size = radius*radius*4;
	if (size > (int)searchOffsets.size()) {
		searchOffsets.resize (size);
		for (int y=0;y<radius*2;y++)
			for (int x=0;x<radius*2;x++){
				SearchOffset& i = searchOffsets[y*radius*2+x];
				i.dx = x-radius;
				i.dy = y-radius;
				i.qdist = i.dx*i.dx+i.dy*i.dy;
			}
			sort(searchOffsets.begin(), searchOffsets.end(), SearchOffsetComparator);
	}
	return searchOffsets;
}

float3 CMap::ClosestBuildSite(const UnitDef* unitdef,float3 pos,float searchRadius,int minDist){
	NLOG("CMap::ClosestBuildSite");
	return pos;
	/*int endr = (int)(searchRadius / (SQUARE_SIZE*2));
	const vector<SearchOffset>& ofs = GetSearchOffsetTable (endr);
	for(vector<SearchOffset>::const_iterator i = ofs.begin(); i != ofs.end(); ++i){
		//
		float3 p(pos.x+i->dx,pos.y,pos.z + i->dy);
		int* e = new int[200];
		int enumber = G->cb->GetFriendlyUnits(e,p,5);
		if (enumber == 0){
			delete [] e;
			return p;
		}else{
			delete [] e;
			continue;
		}
	}

	return UpVector;	*/
}

bool CMap::CheckFloat3(float3 pos){
	NLOG("CMap::CheckFloat3");
	if(pos == UpVector){ //error codes
		return false;
	}else if(pos == ZeroVector){ //error codes
		return false;
	}else if(pos.x > G->cb->GetMapWidth()*8){ // offmap
		return false;
	}else if(pos.z > G->cb->GetMapHeight()*8){ // offmap
		return false;
	}else if(pos.x < 0){ // offmap
		return false;
	}else if(pos.z < 0){ // offmap
		return false;
	}else{
		return true;
	}
}
