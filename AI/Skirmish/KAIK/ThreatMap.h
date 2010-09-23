#ifndef KAIK_THREATMAP_HDR
#define KAIK_THREATMAP_HDR

#include <map>
#include <vector>

struct AIClasses;

class CThreatMap {
public:
	CR_DECLARE(CThreatMap);

	CThreatMap(AIClasses* ai);
	~CThreatMap();

	void PostLoad();
	void Update();

	void EnemyCreated(int);
	void EnemyFinished(int);
	void EnemyDamaged(int, int);
	void EnemyDestroyed(int, int);

	float GetAverageThreat() const { return (currAvgThreat + 1.0f); }
	float ThreatAtThisPoint(const float3&) const;

	float* GetThreatArray() { return &threatCellsRaw[0]; }
	int GetThreatMapWidth() const { return width; }
	int GetThreatMapHeight() const { return height; }

	void ToggleVisOverlay();

private:
	struct EnemyUnit {
		bool operator < (const EnemyUnit& e) const {
			return (id < e.id);
		}

		int id;
		float3 pos;
		float threat;
		float range;
	};

	void AddEnemyUnit(const EnemyUnit&, const float s);
	void DelEnemyUnit(const EnemyUnit&);
	float GetEnemyUnitThreat(const EnemyUnit&) const;

	float currAvgThreat;
	float currMaxThreat;
	float currSumThreat;

	int area;
	int width;
	int height;

	int threatMapTexID;

	std::map<int, EnemyUnit> enemyUnits;
	std::vector<float> threatCellsRaw;
	std::vector<float> threatCellsVis;

	AIClasses* ai;
};

#endif
