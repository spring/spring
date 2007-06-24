
#include "CoverageHandler.h"

CR_BIND(CCoverageHandler ,(NULL,Radar))

CR_REG_METADATA(CCoverageHandler,(
				CR_MEMBER(CovMap),
			    CR_MEMBER(ai),
				CR_ENUM_MEMBER(Type)
				));

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
	return (float)hypot(fabs(A.x - B.x), fabs(A.z - A.z));
}
CCoverageHandler::CCoverageHandler(AIClasses*_ai,CoverageType type):
ai(_ai),
Type(type)
/*RadMap(NULL)*/{
//    RadMap = new unsigned char[(ai->cb->GetMapWidth() / 8) * (ai->cb->GetMapHeight() / 8)];
	if (ai) CovMap.resize((ai->cb->GetMapWidth() / 8) * (ai->cb->GetMapHeight() / 8),0);
//    if(RadMap == NULL)
//    for(int n=0; n<ai->cb->GetMapHeight()/8 * ai->cb->GetMapWidth()/8; n++)
//        RadMap[n] = 0;
}

CCoverageHandler::~CCoverageHandler(){
//	if(RadMap != NULL)
//        delete [] RadMap;
}

void CCoverageHandler::Change(int unit, bool Removed){
    int w = int(ai->cb->GetMapWidth() / 8);
    int h = int(ai->cb->GetMapHeight() / 8);
    int ux = int(ai->cb->GetUnitPos(unit).x / 64);
    int uy = int(ai->cb->GetUnitPos(unit).z / 64);
	if (ai->cb->GetUnitDef(unit)->speed!=0.0) return;
    int r = 0;
	switch (Type) {
		case Radar:r = int(ai->cb->GetUnitDef(unit)->radarRadius / 8.0f);break;
		case Sonar:r = int(ai->cb->GetUnitDef(unit)->sonarRadius / 8.0f);break;
		case RJammer:r = int(ai->cb->GetUnitDef(unit)->jammerRadius / 8.0f);break;
		case SJammer:r = int(ai->cb->GetUnitDef(unit)->sonarJamRadius / 8.0f);break;
		case AntiNuke:{
			const UnitDef* ud = ai->cb->GetUnitDef(unit);
			for (std::vector<UnitDef::UnitDefWeapon>::const_iterator i=ud->weapons.begin();i!=ud->weapons.end();i++) {
				if (i->def->interceptor & 1) {
					int tmpr = int(i->def->coverageRange / 8.0f);
					if (r<tmpr) r = tmpr;
				}
			}
			break;
		}
	}
	if (r<=0) return;
    int minx = ux - r < 0 ? 0 : ux - r;
    int maxx = ux + r >= w ? w-1 : ux + r;
    int miny = uy - r < 0 ? 0 : ux - r;
    int maxy = uy + r >= h ? h-1 : uy + r;
    int x,y;
    if(!Removed)    // New Radar
        for(y = miny; y <= maxy; y++)
            for(x = minx; x <= maxx; x++)
                if(Dist((float)x,(float)y,(float)ux,(float)uy) <= r)
                    CovMap[y * w + x]++;
    else            // Lost Radar
        for(y = miny; y <= maxy; y++)
            for(x = minx; x <= maxx; x++)
                if(Dist((float)x,(float)y,(float)ux,(float)uy) <= r)
                    CovMap[y * w + x]--;
}

int CCoverageHandler::GetCoverage(float3 pos)
{
    int w = int(ai->cb->GetMapWidth() / 8);
    int h = int(ai->cb->GetMapHeight() / 8);
    int bx = int(pos.x / 64.0f);
    int by = int(pos.z / 64.0f);
    float height = ai->cb->GetElevation(bx * 64.0f, by * 64.0f);
	switch (Type) {
		case Radar:
		case RJammer:;break;
		case Sonar:
		case SJammer:if (height>0) return 1000000;
	}
	return CovMap[by * w + bx];
}

float3 CCoverageHandler::NextSite(int builder, const UnitDef* unit, int MaxDist){
    int w = int(ai->cb->GetMapWidth() / 8);
    int h = int(ai->cb->GetMapHeight() / 8);
    int bx = int(ai->cb->GetUnitPos(builder).x / 64.0f);
    int by = int(ai->cb->GetUnitPos(builder).z / 64.0f);
    int r = int(MaxDist / 64.0f);
    int minx = bx - r < 0 ? 0 : bx - r;
    int maxx = bx + r >= w ? w-1 : bx + r;
    int miny = by - r < 0 ? 0 : bx - r;
    int maxy = by + r >= h ? h-1 : by + r;
    int bestvalue = 0;
    int bestx = 0;
    int besty = 0;
//    const unsigned short *RMap = ai->cb->GetRadarMap();
	for(int y=miny; y<=maxy; y++){
        for(int x=minx; x<=maxx; x++){
            float height = ai->cb->GetElevation(x * 64.0f, y * 64.0f);
			bool validPos = false;
			switch (Type) {
				case Radar:
				case RJammer:validPos=true;break;
				case Sonar:
				case SJammer:validPos=height<0;break;
			}
            if(validPos && !CovMap[y * w + x]){
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
