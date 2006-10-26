#ifndef THREATMAP_H
#define THREATMAP_H
/*pragma once removed*/
#include "GlobalAI.h"

class CThreatMap
{
public:
	CThreatMap(AIClasses *ai);
	virtual ~CThreatMap();
	void Create();
	void AddEnemyUnit (int unitid);
	void RemoveEnemyUnit (int unitid);
	void Clear();

	
	float GetAverageThreat();
	float ThreatAtThisPoint(float3 pos);
	float* ThreatArray;
	int ThreatMapHeight;
	int ThreatMapWidth;	
	int ThreatResolution;
private:
	float AverageThreat;
	int TotalCells;
	AIClasses *ai;	


};

#endif /* THREATMAP_H */
