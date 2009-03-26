#ifndef KAIK_THREATMAP_HDR
#define KAIK_THREATMAP_HDR

#include <vector>
struct AIClasses;

class CThreatMap {
	public:
		CR_DECLARE(CThreatMap);

		CThreatMap(AIClasses* ai);
		~CThreatMap();

		void PostLoad();

		void Create();
		void AddEnemyUnit(int unitid);
		void RemoveEnemyUnit(int unitid);
		void Clear();

		float GetAverageThreat();
		float ThreatAtThisPoint(float3 pos);
		std::vector<float> ThreatArray;
		int ThreatMapHeight;
		int ThreatMapWidth;
		int ThreatResolution;

	private:
		float AverageThreat;
		int TotalCells;
		AIClasses* ai;
};


#endif
