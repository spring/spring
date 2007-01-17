#include "ThreatMap.h"


CThreatMap::CThreatMap(AIClasses* ai) {
	this -> ai = ai;
	// divide map resolution by this much (8x8 standard spring resolution)
	ThreatResolution = THREATRES;
	ThreatMapWidth = ai -> cb -> GetMapWidth() / ThreatResolution;
	ThreatMapHeight = ai -> cb -> GetMapHeight() / ThreatResolution;
	TotalCells = ThreatMapWidth * ThreatMapHeight;
	ThreatArray = new float [TotalCells];
}
CThreatMap::~CThreatMap() {
	delete[] ThreatArray;
}


void CThreatMap::Create() {
	// L("Creating threat Array");
	Clear();
	int Enemies [MAXUNITS];
	double totalthreat = 0;
	int NumOfEnemies = ai -> cheat -> GetEnemyUnits(Enemies);

	for (int i = 0; i < NumOfEnemies; i++) {
		// L("adding enemy unit");
		AddEnemyUnit(Enemies[i]);
	}

	for (int i = 0; i < TotalCells; i++) {
		totalthreat += ThreatArray[i];
	}

	AverageThreat = totalthreat / TotalCells;

	for (int i = 0; i < TotalCells; i++){
		ThreatArray[i] += AverageThreat;
	}

	// L("Threat array created!");
}


void CThreatMap::AddEnemyUnit (int unitid) {
	if ((!ai -> cb -> UnitBeingBuilt(unitid)) && (ai -> cheat -> GetUnitDef(unitid) -> weapons.size())) {
		float3 pos = ai -> cheat -> GetUnitPos(unitid);
		int posx = int(pos.x / (8 * ThreatResolution));
		int posy = int(pos.z / (8 * ThreatResolution));
		const UnitDef* Def = ai -> cheat -> GetUnitDef(unitid);

		float Range = (ai -> ut -> GetMaxRange(Def) + 100) / (8 * ThreatResolution);
		float SQRange = Range * Range;
		float DPS = ai -> ut -> GetDPS(Def);

		if (DPS > 2000)
			DPS = 2000;

		for (int myx = int(posx - Range); myx < posx + Range; myx++) {
			if (myx >= 0 && myx < ThreatMapWidth) {
				for (int myy = int(posy - Range); myy < posy + Range; myy++) {
					if ((myy >= 0) && (myy < ThreatMapHeight) && (((posx - myx) * (posx - myx) + (posy - myy) * (posy - myy) - 0.5) <= SQRange)) {
						ThreatArray[myy * ThreatMapWidth + myx] += DPS;
					}
				}
			}
		}
	}
}

void CThreatMap::RemoveEnemyUnit (int unitid) {
	float3 pos = ai -> cheat -> GetUnitPos(unitid);
	int posx = int(pos.x / (8 * ThreatResolution));
	int posy = int(pos.z / (8 * ThreatResolution));

	const UnitDef* Def = ai -> cheat -> GetUnitDef(unitid);
	float Range = ai -> ut -> GetMaxRange(Def) / (8 * ThreatResolution) ;
	float SQRange = Range * Range;
	float DPS = ai -> ut -> GetDPS(Def);

	for (int myx = int(posx - Range); myx < posx + Range; myx++) {
		if (myx >= 0 && myx < ThreatMapWidth) {
			for (int myy = int(posy - Range); myy < posy + Range; myy++) {
				if ((myy >= 0) && (myy < ThreatMapHeight) && (((posx - myx)*(posx - myx) + (posy - myy)*(posy - myy)) <= SQRange)) {
					ThreatArray[myy * ThreatMapWidth + myx] -= DPS;
				}
			}
		}
	}
}

void CThreatMap::Clear() {
	for (int i = 0; i < TotalCells; i++)
		ThreatArray[i] = 0;
}


float CThreatMap::GetAverageThreat() {
	return (AverageThreat + 1);
}


float CThreatMap::ThreatAtThisPoint(float3 pos) {
	return ThreatArray[int(pos.z / (8 * ThreatResolution)) * ThreatMapWidth + int(pos.x / (8 * ThreatResolution))];
}
