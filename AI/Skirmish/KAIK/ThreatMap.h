#ifndef KAIK_THREATMAP_HDR
#define KAIK_THREATMAP_HDR

#include <vector>
struct AIClasses;

class CThreatMap {
public:
	CR_DECLARE(CThreatMap);

	CThreatMap(AIClasses* ai);
	~CThreatMap() {}

	void PostLoad();
	void Update();

	float GetAverageThreat() const;
	float ThreatAtThisPoint(const float3&) const;

	std::vector<float> ThreatArray;
	int ThreatMapHeight;
	int ThreatMapWidth;

private:
	void AddEnemyUnit(int unitid);
	// void RemoveEnemyUnit(int unitid);

	float currAvgThreat;
	float currMaxThreat;
	float currSumThreat;
	int TotalCells;

	AIClasses* ai;
};


#endif
