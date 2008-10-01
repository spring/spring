#ifndef THREATMAP_H
#define THREATMAP_H

#include "GlobalAI.h"

class CThreatMap
{
public:
	CR_DECLARE(CThreatMap);
	CThreatMap(AIClasses *ai);
	virtual ~CThreatMap();
	void PostLoad();
	void Create();
	void AddEnemyUnit (int unitid);
	void RemoveEnemyUnit (int unitid);
	void Clear();

	
	float GetAverageThreat();
	float GetUnmodifiedAverageThreat();
	float ThreatAtThisPoint(const float3 &pos);
	vector<float> ThreatArray;
	vector<float> EmptyThreatArray;
	int ThreatMapHeight;
	int ThreatMapWidth;	
	int ThreatResolution;
private:
	float AverageThreat;
	int TotalCells;
	AIClasses *ai;
//	vector<int> xend;
};

#endif /* THREATMAP_H */
