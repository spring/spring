/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
#include "System/FileSystem/CRC.h"
#include "System/Util.h"

using std::min;
using std::max;

CR_BIND(MoveData, (0));
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
	CR_MEMBER(tempOwner),

	CR_MEMBER(heatMapping),
	CR_MEMBER(heatMod),
	CR_MEMBER(heatProduced),
	CR_MEMBER(flowMapping),
	CR_MEMBER(flowMod),

	CR_RESERVED(16)
));

CR_REG_METADATA(CMoveInfo, (
	CR_MEMBER(moveData),
	CR_MEMBER(name2moveData),
	CR_MEMBER(moveInfoChecksum),
	CR_RESERVED(16)
));


CMoveInfo* moveinfo;

static float DegreesToMaxSlope(float degrees)
{
	return (1.0f - cos(degrees * 1.5f * PI / 180.0f));
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

	CRC crc;

	for (int tt = 0; tt < CMapInfo::NUM_TERRAIN_TYPES; ++tt) {
		const CMapInfo::TerrainType& terrType = mapInfo->terrainTypes[tt];
		crc << terrType.tankSpeed << terrType.kbotSpeed
			<< terrType.hoverSpeed << terrType.shipSpeed;
	}

	for (size_t num = 1; /* no test */; num++) {
		const LuaTable moveTable = rootTable.SubTable(num);
		if (!moveTable.IsValid()) {
			break;
		}

		MoveData* md = new MoveData(NULL);

		md->name     = StringToLower(moveTable.GetString("name", ""));
		md->pathType = (num - 1);
		md->maxSlope = 1.0f;
		md->depth    = 0.0f;
		md->depthMod = 0.0f;
		md->crushStrength = moveTable.GetFloat("crushStrength", 10.0f);

		const float minWaterDepth = moveTable.GetFloat("minWaterDepth", 10.0f);
		const float maxWaterDepth = moveTable.GetFloat("maxWaterDepth", 0.0f);

		if ((md->name.find("boat") != string::npos) ||
		    (md->name.find("ship") != string::npos)) {
			md->moveType   = MoveData::Ship_Move;
			md->depth      = minWaterDepth;
			md->moveFamily = MoveData::Ship;
			md->moveMath   = seaMoveMath;
			md->subMarine  = moveTable.GetBool("subMarine", 0);
		}
		else if (md->name.find("hover") != string::npos) {
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

			if (md->name.find("tank") != string::npos) {
				md->moveFamily = MoveData::Tank;
			} else {
				md->moveFamily = MoveData::KBot;
			}
		}

		md->heatMapping  = moveTable.GetBool("heatMapping", false);
		md->heatMod      = moveTable.GetFloat("heatMod", 50.0f);
		md->heatProduced = moveTable.GetInt("heatProduced", 60);
		md->flowMapping  = moveTable.GetBool("flowMapping", true);
		md->flowMod      = moveTable.GetFloat("flowMod", 1.0f);

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

		const unsigned int checksum =
			(md->size         << 16) +
			(md->followGround << 4) +
			(md->subMarine    << 3) +
			(b2               << 2) +
			(b1               << 1) +
			(b0               << 0);
		crc << checksum
			<< md->maxSlope << md->slopeMod
			<< md->depth << md->depthMod
			<< md->crushStrength;

		moveData.push_back(md);
		name2moveData[md->name] = md->pathType;
	}


	if (mapInfo->water.damage >= 1000.0f) {
		CGroundMoveMath::waterCost = 0.0f;
	} else {
		CGroundMoveMath::waterCost = 1.0f / (1.0f + mapInfo->water.damage * 0.1f);
	}

	CHoverMoveMath::noWaterMove = (mapInfo->water.damage >= 10000.0f);

	crc << CGroundMoveMath::waterCost
		<< CHoverMoveMath::noWaterMove;

	moveInfoChecksum = crc.GetDigest();
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


MoveData* CMoveInfo::GetMoveDataFromName(const std::string& name)
{
	map<string, int>::const_iterator it = name2moveData.find(name);
	if (it == name2moveData.end()) {
		return NULL;
	}
	return moveData[it->second];
}
