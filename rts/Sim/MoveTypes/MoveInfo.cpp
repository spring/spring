#include "StdAfx.h"
#include "MoveInfo.h"
#include "Game/Game.h"
#include "Lua/LuaParser.h"
#include "LogOutput.h"
#include "Map/ReadMap.h"
#include "Map/MapInfo.h"
#include "MoveMath/MoveMath.h"
#include <boost/lexical_cast.hpp>
#include "creg/STL_Deque.h"
#include "creg/STL_Map.h"
#include "mmgr.h"

using std::min;
using std::max;

CR_BIND(MoveData, );
CR_BIND(CMoveInfo, );

CR_REG_METADATA(MoveData, (
		CR_ENUM_MEMBER(moveType),
		CR_MEMBER(size),

		CR_MEMBER(depth),
		CR_MEMBER(maxSlope),
		CR_MEMBER(slopeMod),
		CR_MEMBER(depthMod),

		CR_MEMBER(pathType),
		CR_MEMBER(moveMath),
		CR_MEMBER(crushStrength),
		CR_MEMBER(moveFamily),

		CR_MEMBER(name),
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

	moveInfoChecksum = 0;

	for (size_t num = 1; /* no test */; num++) {
		const LuaTable moveTable = rootTable.SubTable(num);
		if (!moveTable.IsValid()) {
			break;
		}

		MoveData* md = SAFE_NEW MoveData;
		const string name = moveTable.GetString("name", "");

		md->name = name;

		md->pathType = (num - 1);
		md->maxSlope = 1.0f;
		md->depth = 0.0f;
		md->crushStrength = moveTable.GetFloat("crushStrength", 10.0f);

		if ((name.find("BOAT") != string::npos) ||
		    (name.find("SHIP") != string::npos)) {
			md->moveType = MoveData::Ship_Move;
			md->depth = moveTable.GetFloat("minWaterDepth", 10.0f);
			md->moveFamily = 3;
		}
		else if (name.find("HOVER") != string::npos) {
			md->moveType = MoveData::Hover_Move;
			md->maxSlope = DegreesToMaxSlope(moveTable.GetFloat("maxSlope", 15.0f));
			md->moveFamily = 2;
		}
		else {
			md->moveType = MoveData::Ground_Move;
			md->depthMod = 0.1f;
			md->depth = moveTable.GetFloat("maxWaterDepth", 0.0f);
			md->maxSlope = DegreesToMaxSlope(moveTable.GetFloat("maxSlope", 60.0f));
			if (name.find("TANK") != string::npos) {
				md->moveFamily = 0;
			} else {
				md->moveFamily = 1;
			}
		}

		md->slopeMod = 4.0f / (md->maxSlope + 0.001f);

		// TA has only half our res so multiply size with 2
		md->size = max(2, min(8, moveTable.GetInt("footprintX", 1) * 2));

		moveInfoChecksum += md->size;
		moveInfoChecksum ^= *(unsigned int*)&md->slopeMod;
		moveInfoChecksum += *(unsigned int*)&md->depth;

		moveData.push_back(md);
		name2moveData[name] = md->pathType;
	}

	for (int a = 0; a < 256; ++a) {
		terrainType2MoveFamilySpeed[a][0] = mapInfo->terrainTypes[a].tankSpeed;
		terrainType2MoveFamilySpeed[a][1] = mapInfo->terrainTypes[a].kbotSpeed;
		terrainType2MoveFamilySpeed[a][2] = mapInfo->terrainTypes[a].hoverSpeed;
		terrainType2MoveFamilySpeed[a][3] = mapInfo->terrainTypes[a].shipSpeed;
	}
}


CMoveInfo::~CMoveInfo()
{
	while (!moveData.empty()) {
		delete moveData.back();
		moveData.pop_back();
	}
}


MoveData* CMoveInfo::GetMoveDataFromName(const std::string& name, bool exactMatch)
{
	if (!exactMatch) {
		return moveData[name2moveData[name]];
	}
	else {
		map<string, int>::const_iterator it = name2moveData.find(name);
		if (it == name2moveData.end()) {
			return NULL;
		}
		return moveData[it->second];
	}
	return moveData[name2moveData[name]];
}

