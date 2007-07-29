#include "ThreatMap.h"


CThreatMap::CThreatMap(AIClasses* ai) {
	this->ai = ai;
	// divide map resolution by this much (8x8 standard Spring resolution)
	ThreatResolution = THREATRES;
	ThreatMapWidth = ai->cb->GetMapWidth() / ThreatResolution;
	ThreatMapHeight = ai->cb->GetMapHeight() / ThreatResolution;
	TotalCells = ThreatMapWidth * ThreatMapHeight;
	ThreatArray = new float[TotalCells];
}
CThreatMap::~CThreatMap() {
	delete[] ThreatArray;
}


void CThreatMap::Create() {
	Clear();
	int Enemies[MAXUNITS];
	double totalthreat = 0;
	int numEnemies = ai->cheat->GetEnemyUnits(Enemies);

	for (int i = 0; i < numEnemies; i++) {
		AddEnemyUnit(Enemies[i]);
	}

	for (int i = 0; i < TotalCells; i++) {
		totalthreat += ThreatArray[i];
	}

	AverageThreat = totalthreat / TotalCells;

	for (int i = 0; i < TotalCells; i++){
		ThreatArray[i] += AverageThreat;
	}
}


void CThreatMap::AddEnemyUnit(int unitid) {
	const UnitDef* ud = ai->cheat->GetUnitDef(unitid);

	// unarmed units do not register on the threat-map
	if (ud && (!ai->cheat->UnitBeingBuilt(unitid)) && ud->weapons.size()) {
		float3 pos = ai->cheat->GetUnitPos(unitid);
		int posx = int(pos.x / (8 * ThreatResolution));
		int posy = int(pos.z / (8 * ThreatResolution));

		float Range = (ai->ut->GetMaxRange(ud) + 100) / (8 * ThreatResolution);
		float SQRange = Range * Range;
		float DPS = ai->ut->GetDPS(ud);

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
	float3 pos = ai->cheat->GetUnitPos(unitid);
	int posx = int(pos.x / (8 * ThreatResolution));
	int posy = int(pos.z / (8 * ThreatResolution));

	const UnitDef* Def = ai->cheat->GetUnitDef(unitid);
	float Range = ai->ut->GetMaxRange(Def) / (8 * ThreatResolution);
	float SQRange = Range * Range;
	float DPS = ai->ut->GetDPS(Def);

	for (int myx = int(posx - Range); myx < posx + Range; myx++) {
		if (myx >= 0 && myx < ThreatMapWidth) {
			for (int myy = int(posy - Range); myy < posy + Range; myy++) {
				if ((myy >= 0) && (myy < ThreatMapHeight) && (((posx - myx) * (posx - myx) + (posy - myy) * (posy - myy)) <= SQRange)) {
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
