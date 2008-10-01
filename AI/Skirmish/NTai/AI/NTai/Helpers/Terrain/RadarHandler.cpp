#include "../../Core/include.h"


// this class is courtesy fo the OTAI project.
// Written by Veylon, slightly modified by AF.
// Distributed under GPL V2
namespace ntai {
	int mandist(int x1, int y1, int x2, int y2){
		return abs(x1 - x2) + abs(y1 - y2);
	}

	int RevDist(int X1, int Y1, int X2, int Y2, int Max, int Increment){
		int dist = mandist(X1, Y1, X2, Y2);
		if(dist * Increment > Max)
			return 0;
		else
			return Max - dist * Increment;
	}

	float Dist(float x1, float y1, float x2, float y2){
		return (float)hypot(fabs(x1 - x2), fabs(y1-y2));
	}

	float F3Dist(float3 A, float3 B){
		return A.distance2D(B);
	}

	CRadarHandler::CRadarHandler(Global*_gs):
	G(_gs),
	RadMap(NULL){
		RadMap = new unsigned char[(G->cb->GetMapWidth() / 8) * (G->cb->GetMapHeight() / 8)];
		if(RadMap == NULL)
		for(int n=0; n<G->cb->GetMapHeight()/8 * G->cb->GetMapWidth()/8; n++)
			RadMap[n] = 0;
	}

	CRadarHandler::~CRadarHandler(){
		if(RadMap != NULL)
			delete [] RadMap;
	}

	void CRadarHandler::Change(int unit, bool Removed){
		int w = int(G->cb->GetMapWidth() / 8);
		int h = int(G->cb->GetMapHeight() / 8);
		int ux = int(G->cb->GetUnitPos(unit).x / 64);
		int uy = int(G->cb->GetUnitPos(unit).z / 64);
		int r = int(G->cb->GetUnitDef(unit)->radarRadius / 8.0f);
		int minx = ux - r < 0 ? 0 : ux - r;
		int maxx = ux + r >= w ? w-1 : ux + r;
		int miny = uy - r < 0 ? 0 : ux - r;
		int maxy = uy + r >= h ? h-1 : uy + r;
		int x,y;
		if(!Removed)    // New Radar
			for(y = miny; y <= maxy; y++)
				for(x = minx; x <= maxx; x++)
					if(Dist((float)x,(float)y,(float)ux,(float)uy) <= r)
						RadMap[y * w + x]++;
		else            // Lost Radar
			for(y = miny; y <= maxy; y++)
				for(x = minx; x <= maxx; x++)
					if(Dist((float)x,(float)y,(float)ux,(float)uy) <= r)
						RadMap[y * w + x]--;
	}

	float3 CRadarHandler::NextSite(float3 builderpos, const UnitDef* unit, int MaxDist){
		int w = int(G->cb->GetMapWidth() / 8);
		int h = int(G->cb->GetMapHeight() / 8);
		int bx = int(builderpos.x / 64.0f);
		int by = int(builderpos.z / 64.0f);
		int r = int(MaxDist / 64.0f);
		int minx = bx - r < 0 ? 0 : bx - r;
		int maxx = bx + r >= w ? w-1 : bx + r;
		int miny = by - r < 0 ? 0 : bx - r;
		int maxy = by + r >= h ? h-1 : by + r;
		int bestvalue = 0;
		int bestx = 0;
		int besty = 0;
		const unsigned short *RMap = G->cb->GetRadarMap();
		for(int y=miny; y<=maxy; y++){
			for(int x=minx; x<=maxx; x++){
				float height = G->cb->GetElevation(y * 64.0f, x * 64.0f);
				if(height > 0 && !RMap[y * w + x]){
					int value = int(height / 10.0f) - int(mandist(bx,by,x,y)/2.0f);
					if(value > bestvalue){
							bestvalue = value;
							bestx = x;
							besty = y;
					}
				}
			}
		}
		if(bestvalue == 0){
			return UpVector;
		}else{
			return float3(bestx * 64.0f, 0.0f, besty * 64.0f);
		}
	}
}
