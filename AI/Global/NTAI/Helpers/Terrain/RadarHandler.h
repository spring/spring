#ifndef RADAR_HANDLER_H
#define RADAR_HANDLER_H

struct CRadarHandler{
    CRadarHandler(Global*);
	~CRadarHandler();
	
	void Change(int unit);
	float3 NextSite(int builder, const UnitDef* unit, int MaxDist);

	void Change(int unit, bool Removed);
	unsigned char *RadMap;
	
    Global* G;
};
#endif
