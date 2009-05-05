#include "IncExternAI.h"
#include "IncGlobalAI.h"

CR_BIND(CThreatMap, (NULL))
CR_REG_METADATA(CThreatMap, (
	CR_MEMBER(ThreatArray),
	// CR_MEMBER(xend),
	CR_MEMBER(ai),
	CR_RESERVED(8),
	CR_POSTLOAD(PostLoad)
));



CThreatMap::CThreatMap(AIClasses* ai) {
	this->ai = ai;
	// divide map resolution by this much (8x8 standard Spring resolution)
	ThreatResolution = THREATRES;

	if (ai) {
		ThreatMapWidth = ai->cb->GetMapWidth() / ThreatResolution;
		ThreatMapHeight = ai->cb->GetMapHeight() / ThreatResolution;
		TotalCells = ThreatMapWidth * ThreatMapHeight;
		ThreatArray.resize(TotalCells);
	}
}

CThreatMap::~CThreatMap() {
}

void CThreatMap::PostLoad() {
	ThreatMapWidth = ai->cb->GetMapWidth() / ThreatResolution;
	ThreatMapHeight = ai->cb->GetMapHeight() / ThreatResolution;
	TotalCells = ThreatMapWidth * ThreatMapHeight;
}



void CThreatMap::Create() {
	Clear();
	double totalthreat = 0;
	int numEnemies = ai->ccb->GetEnemyUnits(&ai->unitIDs[0]);

	for (int i = 0; i < numEnemies; i++) {
		AddEnemyUnit(ai->unitIDs[i]);
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
	const UnitDef* ud = ai->ccb->GetUnitDef(unitid);

	// unarmed or unfinished units do not register on the threat-map
	if (ud && (!ai->ccb->UnitBeingBuilt(unitid)) && ud->weapons.size()) {
		float3 pos = ai->ccb->GetUnitPos(unitid);
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
	float3 pos = ai->ccb->GetUnitPos(unitid);
	int posx = int(pos.x / (8 * ThreatResolution));
	int posy = int(pos.z / (8 * ThreatResolution));

	const UnitDef* Def = ai->ccb->GetUnitDef(unitid);
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
