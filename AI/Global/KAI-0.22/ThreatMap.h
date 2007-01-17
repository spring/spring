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
	float GetUnmodifiedAverageThreat();
	float ThreatAtThisPoint(const float3 &pos);
	float* ThreatArray;
	float* EmptyThreatArray;
	int ThreatMapHeight;
	int ThreatMapWidth;	
	int ThreatResolution;
private:
	float AverageThreat;
	int TotalCells;
	AIClasses *ai;
	int* xend;


};

#endif /* THREATMAP_H */
