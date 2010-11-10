#include <cassert>
#include <sstream>

#include "IncExternAI.h"
#include "IncGlobalAI.h"

#define LUA_THREATMAP_DEBUG 0

CR_BIND(CThreatMap, (NULL))
CR_REG_METADATA(CThreatMap, (
	CR_MEMBER(threatCellsRaw),
	CR_MEMBER(threatCellsVis),
	CR_MEMBER(ai),
	CR_RESERVED(8),
	CR_POSTLOAD(PostLoad)
));

CThreatMap::CThreatMap(AIClasses* aic): ai(aic) {
	if (ai) {
		PostLoad();

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

CThreatMap::~CThreatMap() {
	threatCellsRaw.clear();
	threatCellsVis.clear();

	if (threatMapTexID >= 0) {
		ai->cb->DebugDrawerDelOverlayTexture(threatMapTexID);
	}
}

void CThreatMap::PostLoad() {
	width  = ai->cb->GetMapWidth() / THREATRES;
	height = ai->cb->GetMapHeight() / THREATRES;
	area   = width * height;

	assert(threatCellsRaw.empty());
	assert(threatCellsVis.empty());
	threatCellsRaw.resize(area, THREATVAL_BASE);
	threatCellsVis.resize(area, THREATVAL_BASE);

	threatMapTexID = -1;
}

void CThreatMap::ToggleVisOverlay() {
	if (threatMapTexID < 0) {
		std::stringstream threatMapLabel;
			threatMapLabel << "[KAIK][";
			threatMapLabel << ai->cb->GetMyTeam();
			threatMapLabel << "][ThreatMap]";

		// /cheat
		// /debugdrawai
		// /team N
		// /spectator
		// "KAIK::ThreatMap::DBG"
		threatMapTexID = ai->cb->DebugDrawerAddOverlayTexture(&threatCellsVis[0], width, height);

		ai->cb->DebugDrawerSetOverlayTexturePos(threatMapTexID, 0.50f, 0.25f);
		ai->cb->DebugDrawerSetOverlayTextureSize(threatMapTexID, 0.40f, 0.40f);
		ai->cb->DebugDrawerSetOverlayTextureLabel(threatMapTexID, (threatMapLabel.str()).c_str());
	} else {
		ai->cb->DebugDrawerDelOverlayTexture(threatMapTexID);
		threatMapTexID = -1;
	}
}



void CThreatMap::EnemyCreated(int enemyUnitID) {
	assert(!threatCellsRaw.empty());
	assert(ai->ccb->GetUnitDef(enemyUnitID) != NULL);

	EnemyUnit enemyUnit;
		enemyUnit.id     = enemyUnitID;
		enemyUnit.pos    = ai->ccb->GetUnitPos(enemyUnitID);
		enemyUnit.threat = GetEnemyUnitThreat(enemyUnit);
		enemyUnit.range  = (ai->ut->GetMaxRange(ai->ccb->GetUnitDef(enemyUnitID)) + 100.0f) / (SQUARE_SIZE * THREATRES);
	enemyUnits[enemyUnitID] = enemyUnit;

	AddEnemyUnit(enemyUnit, 1.0f);
}

void CThreatMap::EnemyFinished(int enemyUnitID) {
	// no-op
}

void CThreatMap::EnemyDamaged(int enemyUnitID, int) {
	assert(!threatCellsRaw.empty());

	std::map<int, EnemyUnit>::iterator it = enemyUnits.find(enemyUnitID);

	if (it != enemyUnits.end()) {
		EnemyUnit& enemyUnit = it->second;

		DelEnemyUnit(enemyUnit);
			enemyUnit.threat = GetEnemyUnitThreat(enemyUnit);
		AddEnemyUnit(enemyUnit, 1.0f);
	}
}

void CThreatMap::EnemyDestroyed(int enemyUnitID, int) {
	assert(!threatCellsRaw.empty());
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

	assert(!threatCellsRaw.empty());
	assert(MAPPOS_IN_BOUNDS(e.pos));

	const float threat = e.threat * s;
	const float rangeSq = e.range * e.range;

	for (int myx = int(posx - e.range); myx < (posx + e.range); myx++) {
		if (myx < 0 || myx >= width) {
			continue;
		}

		for (int myy = int(posy - e.range); myy < (posy + e.range); myy++) {
			if (myy < 0 || myy >= height) {
				continue;
			}

			const int dxSq = (posx - myx) * (posx - myx);
			const int dySq = (posy - myy) * (posy - myy);

			if ((dxSq + dySq - 0.5) <= rangeSq) {
				assert((myy * width + myx) < threatCellsRaw.size());
				assert((myy * width + myx) < threatCellsVis.size());

				// MicroPather cannot deal with negative costs
				// (which may arise due to floating-point drift)
				// nor with zero-cost nodes (see MP::SetMapData,
				// threat is not used as an additive overlay)
				threatCellsRaw[myy * width + myx] = std::max(threatCellsRaw[myy * width + myx] + threat, THREATVAL_BASE);
				threatCellsVis[myy * width + myx] = std::max(threatCellsVis[myy * width + myx] + threat, THREATVAL_BASE);

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

	// TODO: staggered updates
	if (threatMapTexID >= 0) {
		if (currMaxThreat > 0.0f) {
			for (int i = 0; i < area; i++) {
				threatCellsVis[i] = (threatCellsRaw[i] - THREATVAL_BASE) / currMaxThreat;
			}

			ai->cb->DebugDrawerUpdateOverlayTexture(threatMapTexID, &threatCellsVis[0], 0, 0, width, height);
		}
	}

	#if (LUA_THREATMAP_DEBUG == 1)
	{
		std::string luaDataStr;
		std::stringstream luaDataStream;
			luaDataStream << "local threatMapArray = GG.AIThreatMap;\n";

		// just copy the entire map
		for (int i = 0; i < area; i++) {
			luaDataStream << "threatMapArray[" << i << "]";
			luaDataStream << " = ";
			luaDataStream << (threatCellsRaw[i] - THREATVAL_BASE);
			luaDataStream << " / ";
			luaDataStream << currMaxThreat;
			luaDataStream << ";\n";
		}

		luaDataStr = luaDataStream.str();

		ai->cb->CallLuaRules("[AI::KAIK::ThreatMap::Update]", -1, NULL);
		ai->cb->CallLuaRules(luaDataStr.c_str(), -1, NULL);
	}
	#endif
}



float CThreatMap::GetEnemyUnitThreat(const EnemyUnit& e) const {
	const UnitDef* ud = ai->ccb->GetUnitDef(e.id);

	// only unarmed units do not register on the threat-map
	if (ud == NULL || ud->weapons.empty()) {
		return 0.0f;
	}

	const float dps = std::min(ai->ut->GetDPS(ud), 2000.0f);
	const float dpsMod = ai->ccb->GetUnitHealth(e.id) / ai->ccb->GetUnitMaxHealth(e.id);

	return (dps * dpsMod);
}

float CThreatMap::ThreatAtThisPoint(const float3& pos) const {
	const int z = int(pos.z / (SQUARE_SIZE * THREATRES));
	const int x = int(pos.x / (SQUARE_SIZE * THREATRES));
	return (threatCellsRaw[z * width + x] - THREATVAL_BASE);
}
