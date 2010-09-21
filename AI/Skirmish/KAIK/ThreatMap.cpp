#include <sstream>

#include "IncExternAI.h"
#include "IncGlobalAI.h"

#define LUA_THREATMAP_DEBUG 0

CR_BIND(CThreatMap, (NULL))
CR_REG_METADATA(CThreatMap, (
	CR_MEMBER(threatCells),
	CR_MEMBER(ai),
	CR_RESERVED(8),
	CR_POSTLOAD(PostLoad)
));

CThreatMap::CThreatMap(AIClasses* aic): ai(aic) {
	if (ai) {
		width  = ai->cb->GetMapWidth()  / THREATRES;
		height = ai->cb->GetMapHeight() / THREATRES;

		area = width * height;
		threatCells.resize(area);

		#if (LUA_THREATMAP_DEBUG == 1)
		std::stringstream luaDataStream;
			// this approach does not work because Spring's loadstring()
			// function does _not_ run in the global environment, which
			// means transferring the threatmap values requires an extra
			// temporary table
			//
			// luaDataStream << "threatMapW = " << width  << ";\n";
			// luaDataStream << "threatMapH = " << height << ";\n";
			//
			// however, writing to a table declared in GG is fine
			luaDataStream << "GG.AIThreatMap[\"threatMapSizeX\"] = " << width << ";\n";
			luaDataStream << "GG.AIThreatMap[\"threatMapSizeZ\"] = " << height << ";\n";
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

	currMaxThreat = 0.0f; // maximum threat (normalizer)
	currSumThreat = 0.0f; // threat summed over all cells
	currAvgThreat = 0.0f; // average threat over all cells
}

void CThreatMap::PostLoad() {
	width = ai->cb->GetMapWidth() / THREATRES;
	height = ai->cb->GetMapHeight() / THREATRES;
	area = width * height;
}



void CThreatMap::EnemyCreated(int enemyUnitID) {
	const UnitDef* ud = ai->ccb->GetUnitDef(enemyUnitID);

	EnemyUnit enemyUnit;
		enemyUnit.id     = enemyUnitID;
		enemyUnit.pos    = ai->ccb->GetUnitPos(enemyUnitID);
		enemyUnit.threat = GetEnemyUnitThreat(enemyUnit);
		enemyUnit.range  = (ai->ut->GetMaxRange(ud) + 100.0f) / (SQUARE_SIZE * THREATRES);
	enemyUnits[enemyUnitID] = enemyUnit;

	AddEnemyUnit(enemyUnit, 1.0f);
}

void CThreatMap::EnemyFinished(int enemyUnitID) {
	// no-op
}

void CThreatMap::EnemyDamaged(int enemyUnitID, int) {
	std::map<int, EnemyUnit>::iterator it = enemyUnits.find(enemyUnitID);

	if (it != enemyUnits.end()) {
		EnemyUnit& enemyUnit = it->second;

		DelEnemyUnit(enemyUnit);
			enemyUnit.threat = GetEnemyUnitThreat(enemyUnit);
		AddEnemyUnit(enemyUnit, 1.0f);
	}
}

void CThreatMap::EnemyDestroyed(int enemyUnitID, int) {
	std::map<int, EnemyUnit>::const_iterator it = enemyUnits.find(enemyUnitID);

	if (it != enemyUnits.end()) {
		const EnemyUnit& enemyUnit = it->second;

		DelEnemyUnit(enemyUnit);
		enemyUnits.erase(enemyUnitID);
	}
}



void CThreatMap::AddEnemyUnit(const EnemyUnit& e, const float s) {
	const int posx = int(e.pos.x / (SQUARE_SIZE * THREATRES));
	const int posy = int(e.pos.z / (SQUARE_SIZE * THREATRES));

	const float threat = e.threat * s;
	const float rangeSq = e.range * e.range;

	for (int myx = int(posx - e.range); myx < (posx + e.range); myx++) {
		if (myx < 0 || myx >= width) {
			continue;
		}

		for (int myy = int(posy - e.range); myy < (posy + e.range); myy++) {
			if (myy < 0 || myx >= height) {
				continue;
			}

			if (((posx - myx) * (posx - myx) + (posy - myy) * (posy - myy) - 0.5) <= rangeSq) {
				threatCells[myy * width + myx] += threat;
				currSumThreat += threat;
			}
		}
	}

	currAvgThreat = currSumThreat / area;
}

void CThreatMap::DelEnemyUnit(const EnemyUnit& e) {
	AddEnemyUnit(e, -1.0f);
}



void CThreatMap::Update() {
	// reset every frame
	currMaxThreat = 0.0f;

	// account for moving units
	for (std::map<int, EnemyUnit>::iterator it = enemyUnits.begin(); it != enemyUnits.end(); ++it) {
		EnemyUnit& e = it->second;

		DelEnemyUnit(e);
			e.pos    = ai->ccb->GetUnitPos(e.id);
			e.threat = GetEnemyUnitThreat(e);
		AddEnemyUnit(e, 1.0f);

		currMaxThreat = std::max(currMaxThreat, e.threat);
	}


	#if (LUA_THREATMAP_DEBUG == 1)
	std::string luaDataStr;
	std::stringstream luaDataStream;
		luaDataStream << "local threatMapArray = GG.AIThreatMap;\n";
	#endif

	// just copy the entire map
	for (int i = 0; i < area; i++) {
		#if (LUA_THREATMAP_DEBUG == 1)
		luaDataStream << "threatMapArray[" << i << "]";
		luaDataStream << " = ";
		luaDataStream << threatCells[i];
		luaDataStream << " / ";
		luaDataStream << currMaxThreat;
		luaDataStream << ";\n";
		#endif
	}

	#if (LUA_THREATMAP_DEBUG == 1)
	luaDataStr = luaDataStream.str();

	ai->cb->CallLuaRules("[AI::KAIK::ThreatMap::Update]", -1, NULL);
	ai->cb->CallLuaRules(luaDataStr.c_str(), -1, NULL);
	#endif
}



float CThreatMap::GetEnemyUnitThreat(const EnemyUnit& e) const {
	const UnitDef* ud = ai->ccb->GetUnitDef(e.id);

	// only unarmed units do not register on the threat-map
	if (ud == NULL || ud->weapons.empty()) {
		return 0.0f;
	}

	float dps = ai->ut->GetDPS(ud);
	float dpsMod = ai->ccb->GetUnitHealth(e.id) / ai->ccb->GetUnitMaxHealth(e.id);

	if (dps > 2000.0f) {
		dps = 2000.0f;
	}

	return (dps * dpsMod);
}

float CThreatMap::ThreatAtThisPoint(const float3& pos) const {
	const int z = int(pos.z / (SQUARE_SIZE * THREATRES));
	const int x = int(pos.x / (SQUARE_SIZE * THREATRES));
	return threatCells[z * width + x];
}
