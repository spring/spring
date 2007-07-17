
#include "CoverageHandler.h"

#define COVERAGE_SQUARE 4.0
#define COVERAGE_SQUARE2 32.0

CR_BIND(CCoverageHandler ,(NULL,Radar))

CR_REG_METADATA(CCoverageHandler,(
				CR_MEMBER(CovMap),
			    CR_MEMBER(ai),
				CR_ENUM_MEMBER(Type),
				CR_RESERVED(64)
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
	if (ai) CovMap.resize((ai->cb->GetMapWidth() / COVERAGE_SQUARE) * (ai->cb->GetMapHeight() / COVERAGE_SQUARE),0);
//    if(RadMap == NULL)
//    for(int n=0; n<ai->cb->GetMapHeight()/8 * ai->cb->GetMapWidth()/8; n++)
//        RadMap[n] = 0;
}

CCoverageHandler::~CCoverageHandler(){
//	if(RadMap != NULL)
//        delete [] RadMap;
}

void CCoverageHandler::Change(int unit, bool Removed){
    int w = int(ai->cb->GetMapWidth() / COVERAGE_SQUARE);
    int h = int(ai->cb->GetMapHeight() / COVERAGE_SQUARE);
    int ux = int(ai->cb->GetUnitPos(unit).x / COVERAGE_SQUARE2);
    int uy = int(ai->cb->GetUnitPos(unit).z / COVERAGE_SQUARE2);
	if (ai->cb->GetUnitDef(unit)->speed!=0.0) return;
    int r = 0;
	switch (Type) {
		case Radar:r = int(ai->cb->GetUnitDef(unit)->radarRadius / COVERAGE_SQUARE);break;
		case Sonar:r = int(ai->cb->GetUnitDef(unit)->sonarRadius / COVERAGE_SQUARE);break;
		case RJammer:r = int(ai->cb->GetUnitDef(unit)->jammerRadius / COVERAGE_SQUARE);break;
		case SJammer:r = int(ai->cb->GetUnitDef(unit)->sonarJamRadius / COVERAGE_SQUARE);break;
		case AntiNuke:{
			const UnitDef* ud = ai->cb->GetUnitDef(unit);
			for (std::vector<UnitDef::UnitDefWeapon>::const_iterator i=ud->weapons.begin();i!=ud->weapons.end();i++) {
				if (i->def->interceptor & 1) {
					int tmpr = int(i->def->coverageRange / COVERAGE_SQUARE2);
					if (r<tmpr) r = tmpr;
				}
			}
			break;
		}
		case Shield:{
			const UnitDef* ud = ai->cb->GetUnitDef(unit);
			for (std::vector<UnitDef::UnitDefWeapon>::const_iterator i=ud->weapons.begin();i!=ud->weapons.end();i++) {
				if (i->def->isShield) {
					int tmpr = int(i->def->shieldRadius / COVERAGE_SQUARE2);
					if (r<tmpr) r = tmpr;
				}
			}
			break;
		}
	}
	if (r<=0) return;
	L("Coverage handler " << Type << ":Unit " << unit << "(" << ai->cb->GetUnitDef(unit)->humanName << ")" << (Removed?" Removed":" Added") << ux << " " << uy << "Range " << r);
    int minx = ux - r < 0 ? 0 : ux - r;
    int maxx = ux + r >= w ? w-1 : ux + r;
    int miny = uy - r < 0 ? 0 : ux - r;
    int maxy = uy + r >= h ? h-1 : uy + r;
    int x,y;
    for(y = miny; y <= maxy; y++)
        for(x = minx; x <= maxx; x++)
            if(Dist((float)x,(float)y,(float)ux,(float)uy) <= r)
			    if(!Removed)    // New Radar
					CovMap[y * w + x]++;
			    else            // Lost Radar
				    CovMap[y * w + x]--;
}

int CCoverageHandler::GetCoverage(float3 pos)
{
    int w = int(ai->cb->GetMapWidth()/COVERAGE_SQUARE);
    int h = int(ai->cb->GetMapHeight()/COVERAGE_SQUARE);
    int bx = int(pos.x / COVERAGE_SQUARE2);
    int by = int(pos.z / COVERAGE_SQUARE2);
    float height = ai->cb->GetElevation(bx * COVERAGE_SQUARE2, by * COVERAGE_SQUARE2);
	switch (Type) {
		case Sonar:
		case SJammer:if (height>0) return 1000000;
	}
	if (bx<0||by<0||bx>=w||by>=h) return 1000000;
	return CovMap[by * w + bx];
}

float3 CCoverageHandler::NextSite(int builder, const UnitDef* unit, int MaxDist){
    int w = int(ai->cb->GetMapWidth()/COVERAGE_SQUARE);
    int h = int(ai->cb->GetMapHeight()/COVERAGE_SQUARE);
    int bx = int(ai->cb->GetUnitPos(builder).x / COVERAGE_SQUARE2);
    int by = int(ai->cb->GetUnitPos(builder).z / COVERAGE_SQUARE2);
    int r = int(MaxDist / COVERAGE_SQUARE2);
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
            float height = ai->cb->GetElevation(x * COVERAGE_SQUARE2, y * COVERAGE_SQUARE2);
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
		return float3(bestx * COVERAGE_SQUARE2, 0.0f, besty * COVERAGE_SQUARE2);
	}
}
