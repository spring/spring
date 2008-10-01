// Map
#include "../../Core/include.h"

namespace ntai {
	CMap::CMap(Global* GL){
		G = GL;
	}

	float3 CMap::distfrom(float3 Start, float3 Target, float distance){
		NLOG("CMap::distfrom");

		if(!CheckFloat3(Start)){
			return UpVector;
		}

		if(!CheckFloat3(Target)){
			return UpVector;
		}

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

	}

	t_direction CMap::WhichCorner(float3 pos){
		NLOG("CMap::WhichCorner");

		if ((pos.x<(G->cb->GetMapWidth()*4))&&(pos.z<(G->cb->GetMapHeight()*SQUARE_SIZE*4))){
			return t_NW;
		}

		if ((pos.x>(G->cb->GetMapWidth()*4))&&(pos.z<(G->cb->GetMapHeight()*SQUARE_SIZE*4))){
			return t_NE;
		}

		if ((pos.x<(G->cb->GetMapWidth()*4))&&(pos.z>(G->cb->GetMapHeight()*SQUARE_SIZE*4))){
			return t_SW;
		}

		if ((pos.x>(G->cb->GetMapWidth()*4))&&(pos.z>(G->cb->GetMapHeight()*SQUARE_SIZE*4))){
			return t_SE;
		}

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

	/*vector<float3> CMap::GetSurroundings(float3 pos){
		NLOG("CMap::GetSurroundings");
		vector<float3> s;
		float3 ss = pos;
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
	}*/

	// ||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||

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
		int x2 = min(G->cb->GetMapWidth()*SQUARE_SIZE,x1+unitdef->xsize);
		int z1 = (int)max(0.f,(pos.z-(unitdef->ysize*0.5f*SQUARE_SIZE))/SQUARE_SIZE);
		int z2 = min(G->cb->GetMapHeight()*SQUARE_SIZE,z1+unitdef->ysize);

		if (x1 > G->cb->GetMapWidth()*SQUARE_SIZE) x1 = G->cb->GetMapWidth()*SQUARE_SIZE;
		if (x2 < 0) x2 = 0;
		if (z1 > G->cb->GetMapHeight()*SQUARE_SIZE) z1 = G->cb->GetMapHeight()*SQUARE_SIZE;
		if (z2 < 0) z2 = 0;

		const float* heightmap = G->cb->GetHeightMap();
		for(int x=x1; x<=x2; x++){
			for(int z=z1; z<=z2; z++){
				float orgh=heightmap[z*(G->cb->GetMapWidth()*SQUARE_SIZE+1)+x];
				float h=heightmap[z*(G->cb->GetMapWidth()*SQUARE_SIZE+1)+x];
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


	bool CMap::CheckFloat3(float3 pos){
		NLOG("CMap::CheckFloat3");
		if(pos == UpVector){ //error codes
			return false;
		}else if(pos == ZeroVector){ //error codes
			return false;
		}else if(pos == float3(-1,0,0)){ //error codes
			return false;
		}else if(pos.distance2D(UpVector) <50){ // top corner!!!!!
			return false;
		}/*else if(pos.x > G->cb->GetMapWidth()*SQUARE_SIZE){ // offmap
			return false;
		}else if(pos.z > G->cb->GetMapHeight()*SQUARE_SIZE){ // offmap
			return false;
		}else if(pos.x < 0){ // offmap
			return false;
		}else if(pos.z < 0){ // offmap
			return false;
		}*/else{
			return true;
		}
	}

	float CMap::GetAngle(float3 origin, float3 a, float3 b){
		float3 c = a-origin;
		float3 d = b-origin;
		c.Normalize();
		d.Normalize();
		return (float)abs( (long)acos( c.dot(d) ) );
	}
}
