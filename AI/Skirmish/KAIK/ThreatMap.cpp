#include <sstream>

#include "IncExternAI.h"
#include "IncGlobalAI.h"

#define LUA_THREATMAP_DEBUG 0

CR_BIND(CThreatMap, (NULL))
CR_REG_METADATA(CThreatMap, (
	CR_MEMBER(ThreatArray),
	CR_MEMBER(ai),
	CR_RESERVED(8),
	CR_POSTLOAD(PostLoad)
));

CThreatMap::CThreatMap(AIClasses* aic): ai(aic) {
	if (ai) {
		ThreatMapWidth  = ai->cb->GetMapWidth()  / THREATRES;
		ThreatMapHeight = ai->cb->GetMapHeight() / THREATRES;
		TotalCells = ThreatMapWidth * ThreatMapHeight;
		ThreatArray.resize(TotalCells);

		#if (LUA_THREATMAP_DEBUG == 1)
		std::stringstream luaDataStream;
			// this approach does not work because Spring's loadstring()
			// function does _not_ run in the global environment, which
			// means transferring the threatmap values requires an extra
			// temporary table
			//
			// luaDataStream << "threatMapW = " << ThreatMapWidth  << ";\n";
			// luaDataStream << "threatMapH = " << ThreatMapHeight << ";\n";
			//
			// however, writing to a table declared in GG is fine
			luaDataStream << "GG.AIThreatMap[\"threatMapSizeX\"] = " << ThreatMapWidth << ";\n";
			luaDataStream << "GG.AIThreatMap[\"threatMapSizeZ\"] = " << ThreatMapHeight << ";\n";
			luaDataStream << "GG.AIThreatMap[\"threatMapResX\"]  = " << THREATRES << ";\n";
			luaDataStream << "GG.AIThreatMap[\"threatMapResZ\"]  = " << THREATRES << ";\n";
			luaDataStream << "\n";
			luaDataStream << "local threatMapSizeX = GG.AIThreatMap[\"threatMapSizeX\"];\n";
			luaDataStream << "local threatMapSizeZ = GG.AIThreatMap[\"threatMapSizeZ\"];\n";
			luaDataStream << "local threatMapArray = GG.AIThreatMap;\n";
			luaDataStream << "\n";
			luaDataStream << "for row = 0, (threatMapSizeZ - 1) do\n";
			luaDataStream << "\tfor col = 0, (threatMapSizeX - 1) do\n";
			luaDataStream << "\t\tthreatMapArray[row * threatMapSizeX + col] = 0.0;\n";
			luaDataStream << "\tend\n";
			luaDataStream << "end\n";
		std::string luaDataStr = luaDataStream.str();

		ai->cb->CallLuaRules("[AI::KAIK::ThreatMap::Init]", -1, NULL);
		ai->cb->CallLuaRules(luaDataStr.c_str(), -1, NULL);
		#endif
	}
}

void CThreatMap::PostLoad() {
	ThreatMapWidth = ai->cb->GetMapWidth() / THREATRES;
	ThreatMapHeight = ai->cb->GetMapHeight() / THREATRES;
	TotalCells = ThreatMapWidth * ThreatMapHeight;
}



void CThreatMap::Update() {
	for (int i = 0; i < TotalCells; i++) {
		ThreatArray[i] = 0.0f;
	}

	currMaxThreat = 0.0f; // maximum threat (normalizer)
	currSumThreat = 0.0f; // threat summed over all cells

	const int numEnemies = ai->ccb->GetEnemyUnits(&ai->unitIDs[0]);

	for (int i = 0; i < numEnemies; i++) {
		AddEnemyUnit(ai->unitIDs[i]);
	}

	for (int i = 0; i < TotalCells; i++) {
		currSumThreat += ThreatArray[i];
		currMaxThreat = std::max(ThreatArray[i], currMaxThreat);
	}

	currAvgThreat = currSumThreat / TotalCells;


	#if (LUA_THREATMAP_DEBUG == 1)
	std::string luaDataStr;
	std::stringstream luaDataStream;
		luaDataStream << "local threatMapArray = GG.AIThreatMap;\n";
	#endif

	for (int i = 0; i < TotalCells; i++) {
		#if (LUA_THREATMAP_DEBUG == 1)
		luaDataStream << "threatMapArray[" << i << "]";
		luaDataStream << " = ";
		luaDataStream << ThreatArray[i];
		luaDataStream << " / ";
		luaDataStream << currMaxThreat;
		luaDataStream << ";\n";
		#endif

		ThreatArray[i] += currAvgThreat; // ?
	}

	#if (LUA_THREATMAP_DEBUG == 1)
	luaDataStr = luaDataStream.str();

	// note: the gadget would need to reset its threatmap table to stay
	// in sync anyway (because the values are re-initialized to 0 here)
	// if we were to only send the deltas, so just copy the entire map
	ai->cb->CallLuaRules("[AI::KAIK::ThreatMap::Update]", -1, NULL);
	ai->cb->CallLuaRules(luaDataStr.c_str(), -1, NULL);
	#endif
}


void CThreatMap::AddEnemyUnit(int unitID) {
	const UnitDef* ud = ai->ccb->GetUnitDef(unitID);

	// only unarmed units do not register on the threat-map
	if (ud && !ud->weapons.empty()) {
		const float3& pos = ai->ccb->GetUnitPos(unitID);
		const int posx = int(pos.x / (8.0f * THREATRES));
		const int posy = int(pos.z / (8.0f * THREATRES));

		const float Range = (ai->ut->GetMaxRange(ud) + 100.0f) / (8.0f * THREATRES);
		const float SQRange = Range * Range;

		float DPS = ai->ut->GetDPS(ud);
		float DPSmod = ai->ccb->GetUnitHealth(unitID) / ai->ccb->GetUnitMaxHealth(unitID);

		if (DPS > 2000.0f)
			DPS = 2000.0f;

		for (int myx = int(posx - Range); myx < (posx + Range); myx++) {
			if (myx >= 0 && myx < ThreatMapWidth) {
				for (int myy = int(posy - Range); myy < (posy + Range); myy++) {
					if ((myy >= 0) && (myy < ThreatMapHeight) && (((posx - myx) * (posx - myx) + (posy - myy) * (posy - myy) - 0.5) <= SQRange)) {
						ThreatArray[myy * ThreatMapWidth + myx] += (DPS * DPSmod);
					}
				}
			}
		}
	}
}

/*
void CThreatMap::RemoveEnemyUnit(int unitid) {
	float3 pos = ai->ccb->GetUnitPos(unitid);
	int posx = int(pos.x / (8 * THREATRES));
	int posy = int(pos.z / (8 * THREATRES));

	const UnitDef* Def = ai->ccb->GetUnitDef(unitid);
	float Range = ai->ut->GetMaxRange(Def) / (8 * THREATRES);
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
*/

float CThreatMap::GetAverageThreat() const {
	return (currAvgThreat + 1.0f);
}

float CThreatMap::ThreatAtThisPoint(const float3& pos) const {
	return ThreatArray[int(pos.z / (8 * THREATRES)) * ThreatMapWidth + int(pos.x / (8 * THREATRES))];
}
