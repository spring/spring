#include "StdAfx.h"
#include <boost/lexical_cast.hpp>
#include "mmgr.h"

#include "MoveInfo.h"
#include "Game/Game.h"
#include "Lua/LuaParser.h"
#include "LogOutput.h"
#include "Map/ReadMap.h"
#include "Map/MapInfo.h"
#include "MoveMath/MoveMath.h"
#include "MoveMath/GroundMoveMath.h"
#include "MoveMath/HoverMoveMath.h"
#include "MoveMath/ShipMoveMath.h"
#include "creg/STL_Deque.h"
#include "creg/STL_Map.h"
#include "Exceptions.h"

using std::min;
using std::max;

CR_BIND(MoveData, (0x0, 0));
CR_BIND(CMoveInfo, );

CR_REG_METADATA(MoveData, (
	CR_ENUM_MEMBER(moveType),
	CR_ENUM_MEMBER(moveFamily),
	CR_ENUM_MEMBER(terrainClass),
	CR_MEMBER(followGround),

	CR_MEMBER(size),
	CR_MEMBER(depth),
	CR_MEMBER(maxSlope),
	CR_MEMBER(slopeMod),
	CR_MEMBER(depthMod),

	CR_MEMBER(pathType),
	CR_MEMBER(moveMath),
	CR_MEMBER(crushStrength),

	CR_MEMBER(name),

	CR_MEMBER(maxSpeed),
	CR_MEMBER(maxTurnRate),

	CR_MEMBER(maxAcceleration),
	CR_MEMBER(maxBreaking),

	CR_MEMBER(subMarine),

	CR_RESERVED(16)
));

CR_REG_METADATA(CMoveInfo, (
	CR_MEMBER(moveData),
	CR_MEMBER(name2moveData),
	CR_MEMBER(moveInfoChecksum),
	CR_MEMBER(terrainType2MoveFamilySpeed),
	CR_RESERVED(16)
));


CMoveInfo* moveinfo;

static float DegreesToMaxSlope(float degrees)
{
	return (float)(1.0 - cos(degrees * 1.5f * PI / 180.0f));
}


CMoveInfo::CMoveInfo()
{
	const LuaTable rootTable = game->defsParser->GetRoot().SubTable("MoveDefs");
	if (!rootTable.IsValid()) {
		throw content_error("Error loading movement definitions");
	}

	groundMoveMath = new CGroundMoveMath();
	hoverMoveMath = new CHoverMoveMath();
	seaMoveMath = new CShipMoveMath();

	moveInfoChecksum = 0;

	for (size_t num = 1; /* no test */; num++) {
		const LuaTable moveTable = rootTable.SubTable(num);
		if (!moveTable.IsValid()) {
			break;
		}

		MoveData* md = new MoveData(0x0, 0);

		md->name     = moveTable.GetString("name", "");
		md->pathType = (num - 1);
		md->maxSlope = 1.0f;
		md->depth    = 0.0f;
		md->depthMod = 0.0f;
		md->crushStrength = moveTable.GetFloat("crushStrength", 10.0f);

		const float minWaterDepth = moveTable.GetFloat("minWaterDepth", 10.0f);
		const float maxWaterDepth = moveTable.GetFloat("maxWaterDepth", 0.0f);

		if ((md->name.find("BOAT") != string::npos) ||
		    (md->name.find("SHIP") != string::npos)) {
			md->moveType   = MoveData::Ship_Move;
			md->depth      = minWaterDepth;
			md->moveFamily = MoveData::Ship;
			md->moveMath   = seaMoveMath;
			md->subMarine  = moveTable.GetBool("subMarine", 0);
		}
		else if (md->name.find("HOVER") != string::npos) {
			md->moveType   = MoveData::Hover_Move;
			md->maxSlope   = DegreesToMaxSlope(moveTable.GetFloat("maxSlope", 15.0f));
			md->moveFamily = MoveData::Hover;
			md->moveMath   = hoverMoveMath;
		}
		else {
			md->moveType = MoveData::Ground_Move;
			md->depthMod = moveTable.GetFloat("depthMod", 0.1f);
			md->depth    = maxWaterDepth;
			md->maxSlope = DegreesToMaxSlope(moveTable.GetFloat("maxSlope", 60.0f));
			md->moveMath = groundMoveMath;

			if (md->name.find("TANK") != string::npos) {
				md->moveFamily = MoveData::Tank;
			} else {
				md->moveFamily = MoveData::KBot;
			}
		}

		// ground units hug the ocean floor when in water,
		// ships stay at a "fixed" level (their waterline)
		md->followGround =
			(md->moveFamily == MoveData::Tank ||
			md->moveFamily == MoveData::KBot);

		// tank or bot that cannot get its threads / feet
		// wet, or hovercraft (which doesn't touch ground
		// or water)
		const bool b0 =
			((md->followGround && maxWaterDepth <= 0.0) ||
			md->moveFamily == MoveData::Hover);

		// ship (or sub) that cannot crawl onto shore, OR tank or
		// kbot restricted to snorkling (strange but possible)
		const bool b1 =
			((md->moveFamily == MoveData::Ship && minWaterDepth > 0.0) ||
			((md->followGround) && minWaterDepth > 0.0));

		// tank or kbot that CAN go skinny-dipping (amph.),
		// or ship that CAN sprout legs when at the beach
		const bool b2 =
			((md->followGround) && maxWaterDepth > 0.0) ||
			(md->moveFamily == MoveData::Ship && minWaterDepth < 0.0);

		if (b0) { md->terrainClass = MoveData::Land; }
		if (b1) { md->terrainClass = MoveData::Water; }
		if (b2) { md->terrainClass = MoveData::Mixed; }


		md->slopeMod = moveTable.GetFloat("slopeMod", 4.0f / (md->maxSlope + 0.001f));
		// TA has only half our resolution, multiply size by 2
		md->size = max(2, min(8, moveTable.GetInt("footprintX", 1) * 2));

		moveInfoChecksum +=
			(md->size         << 5) +
			(md->followGround << 4) +
			(md->subMarine    << 3) +
			(b2               << 2) +
			(b1               << 1) +
			(b0               << 0);
		moveInfoChecksum = moveInfoChecksum * 3 + *(unsigned int*) &md->maxSlope;
		moveInfoChecksum = moveInfoChecksum * 3 + *(unsigned int*) &md->slopeMod;
		moveInfoChecksum = moveInfoChecksum * 3 + *(unsigned int*) &md->depth;
		moveInfoChecksum = moveInfoChecksum * 3 + *(unsigned int*) &md->depthMod;
		moveInfoChecksum = moveInfoChecksum * 5 + *(unsigned int*) &md->crushStrength;

		moveData.push_back(md);
		name2moveData[md->name] = md->pathType;
	}

	for (int a = 0; a < 256; ++a) {
		terrainType2MoveFamilySpeed[a][0] = mapInfo->terrainTypes[a].tankSpeed;
		terrainType2MoveFamilySpeed[a][1] = mapInfo->terrainTypes[a].kbotSpeed;
		terrainType2MoveFamilySpeed[a][2] = mapInfo->terrainTypes[a].hoverSpeed;
		terrainType2MoveFamilySpeed[a][3] = mapInfo->terrainTypes[a].shipSpeed;
	}


	const float waterDamage = mapInfo->water.damage;
	if (waterDamage >= 1000.0f) {
		CGroundMoveMath::waterCost = 0.0f;
	} else {
		CGroundMoveMath::waterCost = 1.0f / (1.0f + waterDamage * 0.1f);
	}

	CHoverMoveMath::noWaterMove = (waterDamage >= 10000.0f);
}


CMoveInfo::~CMoveInfo()
{
	while (!moveData.empty()) {
		delete moveData.back();
		moveData.pop_back();
	}

	delete groundMoveMath;
	delete hoverMoveMath;
	delete seaMoveMath;
}


MoveData* CMoveInfo::GetMoveDataFromName(const std::string& name, bool exactMatch)
{
	if (!exactMatch) {
		return moveData[name2moveData[name]];
	} else {
		map<string, int>::const_iterator it = name2moveData.find(name);
		if (it == name2moveData.end()) {
			return NULL;
		}
		return moveData[it->second];
	}
	return moveData[name2moveData[name]];
}
