/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/lexical_cast.hpp>
#include "System/mmgr.h"

#include "MoveInfo.h"
#include "Game/Game.h"
#include "Lua/LuaParser.h"
#include "Map/ReadMap.h"
#include "Map/MapInfo.h"
#include "MoveMath/MoveMath.h"
#include "MoveMath/GroundMoveMath.h"
#include "MoveMath/HoverMoveMath.h"
#include "MoveMath/ShipMoveMath.h"
#include "System/creg/STL_Deque.h"
#include "System/creg/STL_Map.h"
#include "System/Exceptions.h"
#include "System/CRC.h"
#include "System/myMath.h"
#include "System/Util.h"

CR_BIND(MoveData, (0));
CR_BIND(CMoveInfo, );

CR_REG_METADATA(MoveData, (
	CR_MEMBER(name),

	CR_ENUM_MEMBER(moveType),
	CR_ENUM_MEMBER(moveFamily),
	CR_ENUM_MEMBER(terrainClass),

	CR_MEMBER(xsize),
	CR_MEMBER(xsizeh),
	CR_MEMBER(zsize),
	CR_MEMBER(zsizeh),
	CR_MEMBER(depth),
	CR_MEMBER(depthModParams),
	CR_MEMBER(maxSlope),
	CR_MEMBER(slopeMod),
	CR_MEMBER(crushStrength),

	CR_MEMBER(pathType),
	CR_MEMBER(unitDefRefCount),

	CR_MEMBER(followGround),
	CR_MEMBER(subMarine),

	CR_MEMBER(heatMapping),
	CR_MEMBER(heatMod),
	CR_MEMBER(heatProduced),

	CR_MEMBER(moveMath),
	CR_MEMBER(tempOwner),

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
	// Prevent MSVC from inlining stuff that would break the
	// PE checksum compatibility between debug and release
	static const float degToRad = PI / 180.0f;

	const float deg = Clamp(degrees, 0.0f, 60.0f) * 1.5f;
	const float rad = deg * degToRad;

	return (1.0f - cos(rad));
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

		crc << terrType.tankSpeed << terrType.kbotSpeed;
		crc << terrType.hoverSpeed << terrType.shipSpeed;
	}

	for (size_t num = 1; /* no test */; num++) {
		const LuaTable moveTable = rootTable.SubTable(num);
		if (!moveTable.IsValid()) {
			break;
		}

		MoveData* md = new MoveData(this, moveTable, num);
		moveData.push_back(md);
		name2moveData[md->name] = md->pathType;

		switch (md->moveType) {
			case MoveData::Ship_Move: { md->moveMath = seaMoveMath; } break;
			case MoveData::Hover_Move: { md->moveMath = hoverMoveMath; } break;
			case MoveData::Ground_Move: { md->moveMath = groundMoveMath; } break;
		}

		crc << md->GetCheckSum();
	}


	// FIXME: do something with these magic numbers
	if (mapInfo->water.damage >= 1000.0f) {
		CGroundMoveMath::waterDamageCost = 0.0f; // block water
	} else {
		CGroundMoveMath::waterDamageCost = 1.0f / (1.0f + mapInfo->water.damage * 0.1f);
	}

	CHoverMoveMath::noWaterMove = (mapInfo->water.damage >= 10000.0f);

	crc << CGroundMoveMath::waterDamageCost;
	crc << CHoverMoveMath::noWaterMove;

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



MoveData::MoveData() {
	name              = "";

	moveType          = MoveData::Ground_Move;
	moveFamily        = MoveData::Tank;
	terrainClass      = MoveData::Mixed;

	xsize             = 0;
	zsize             = 0;
	xsizeh            = 0;
	zsizeh            = 0;

	depth             = 0.0f;
	maxSlope          = 1.0f;
	slopeMod          = 0.0f;

	depthModParams[DEPTHMOD_MIN_HEIGHT] = 0.0f;
	depthModParams[DEPTHMOD_MAX_HEIGHT] = std::numeric_limits<float>::max();
	depthModParams[DEPTHMOD_MAX_SCALE ] = std::numeric_limits<float>::max();
	depthModParams[DEPTHMOD_QUA_COEFF ] = 0.0f;
	depthModParams[DEPTHMOD_LIN_COEFF ] = 0.1f;
	depthModParams[DEPTHMOD_CON_COEFF ] = 1.0f;

	crushStrength     = 0.0f;

	pathType          = 0;
	unitDefRefCount   = 0;

	followGround      = true;
	subMarine         = false;

	heatMapping       = true;
	heatMod           = 0.05f;
	heatProduced      = GAME_SPEED;

	moveMath          = NULL;
	tempOwner         = NULL;
}

MoveData::MoveData(const MoveData* unitDefMD) {
	*this = *unitDefMD;
}

MoveData::MoveData(CMoveInfo* moveInfo, const LuaTable& moveTable, int moveDefID) {
	*this = MoveData();

	name          = StringToLower(moveTable.GetString("name", ""));
	pathType      = moveDefID - 1;
	crushStrength = moveTable.GetFloat("crushStrength", 10.0f);

	const float minWaterDepth = moveTable.GetFloat("minWaterDepth", 10.0f);
	const float maxWaterDepth = moveTable.GetFloat("maxWaterDepth", 0.0f);

	if ((name.find("boat") != string::npos) ||
	    (name.find("ship") != string::npos)) {
		moveType   = MoveData::Ship_Move;
		depth      = minWaterDepth;
		moveFamily = MoveData::Ship;
		subMarine  = moveTable.GetBool("subMarine", 0);
	} else if (name.find("hover") != string::npos) {
		moveType   = MoveData::Hover_Move;
		maxSlope   = DegreesToMaxSlope(moveTable.GetFloat("maxSlope", 15.0f));
		moveFamily = MoveData::Hover;
	} else {
		moveType = MoveData::Ground_Move;
		depth    = maxWaterDepth;

		depthModParams[DEPTHMOD_MIN_HEIGHT] = std::max(0.00f, moveTable.GetFloat("depthModMinHeight",                                     0.0f ));
		depthModParams[DEPTHMOD_MAX_HEIGHT] =         (       moveTable.GetFloat("depthModMaxHeight",        std::numeric_limits<float>::max() ));
		depthModParams[DEPTHMOD_MAX_SCALE ] = std::max(0.01f, moveTable.GetFloat("depthModMaxScale",         std::numeric_limits<float>::max() ));
		depthModParams[DEPTHMOD_QUA_COEFF ] = std::max(0.00f, moveTable.GetFloat("depthModQuadraticCoeff",                                0.0f ));
		depthModParams[DEPTHMOD_LIN_COEFF ] = std::max(0.00f, moveTable.GetFloat("depthModLinearCoeff",    moveTable.GetFloat("depthMod", 0.1f)));
		depthModParams[DEPTHMOD_CON_COEFF ] = std::max(0.00f, moveTable.GetFloat("depthModConstantCoeff",                                 1.0f ));

		// ensure [depthModMinHeight, depthModMaxHeight] is a valid range
		depthModParams[DEPTHMOD_MAX_HEIGHT] = std::max(depthModParams[DEPTHMOD_MIN_HEIGHT], depthModParams[DEPTHMOD_MAX_HEIGHT]);

		maxSlope = DegreesToMaxSlope(moveTable.GetFloat("maxSlope", 60.0f));

		if (name.find("tank") != string::npos) {
			moveFamily = MoveData::Tank;
		} else {
			moveFamily = MoveData::KBot;
		}
	}

	heatMapping = moveTable.GetBool("heatMapping", false);
	heatMod = moveTable.GetFloat("heatMod", 50.0f);
	heatProduced = moveTable.GetInt("heatProduced", 60);

	//  <maxSlope> ranges from 0.0 to 60 * 1.5 degrees, ie. from 0.0 to
	//  0.5 * PI radians, ie. from 1.0 - cos(0.0) to 1.0 - cos(0.5 * PI)
	//  = [0, 1] --> DEFAULT <slopeMod> values range from (4 / 0.001) to
	//  (4 / 1.001) = [4000.0, 3.996]
	//
	// speedMod values for a terrain-square slope in [0, 1] are given by
	// (1.0 / (1.0 + slope * slopeMod)) and therefore have a MAXIMUM at
	// <slope=0, slopeMod=...> and a MINIMUM at <slope=1, slopeMod=4000>
	// (of 1.0 / (1.0 + 0.0 * ...) = 1.0 and 1.0 / (1.0 + 1.0 * 4000.0)
	// = 0.00025 respectively)
	//
	slopeMod = moveTable.GetFloat("slopeMod", 4.0f / (maxSlope + 0.001f));

	// ground units hug the ocean floor when in water,
	// ships stay at a "fixed" level (their waterline)
	followGround =
		(moveFamily == MoveData::Tank ||
		 moveFamily == MoveData::KBot);

	// tank or bot that cannot get its threads / feet
	// wet, or hovercraft (which doesn't touch ground
	// or water)
	const bool b0 = ((followGround && maxWaterDepth <= 0.0) || moveFamily == MoveData::Hover);

	// ship (or sub) that cannot crawl onto shore, OR tank or
	// kbot restricted to snorkling (strange but possible)
	const bool b1 = ((moveFamily == MoveData::Ship && minWaterDepth > 0.0) || ((followGround) && minWaterDepth > 0.0));

	// tank or kbot that CAN go skinny-dipping (amph.),
	// or ship that CAN sprout legs when at the beach
	const bool b2 = ((followGround) && maxWaterDepth > 0.0) || (moveFamily == MoveData::Ship && minWaterDepth < 0.0);

	if (b0) { terrainClass = MoveData::Land;  }
	if (b1) { terrainClass = MoveData::Water; }
	if (b2) { terrainClass = MoveData::Mixed; }

	const int xsizeDef = std::max(1, moveTable.GetInt("footprintX",        1));
	const int zsizeDef = std::max(1, moveTable.GetInt("footprintZ", xsizeDef));
	const int scale    = 2;

	// make all mobile footprints point-symmetric in heightmap space
	// (meaning that only non-even dimensions are possible and each
	// footprint always has a unique center square)
	xsize = xsizeDef * scale;
	zsize = zsizeDef * scale;
	xsize -= ((xsize & 1)? 0: 1);
	zsize -= ((zsize & 1)? 0: 1);
	// precalculated data for MoveMath
	xsizeh = xsize >> 1;
	zsizeh = zsize >> 1;
	assert((xsize & 1) == 1);
	assert((zsize & 1) == 1);
}

float MoveData::GetDepthMod(const float height) const {
	// [DEPTHMOD_{MIN, MAX}_HEIGHT] are always >= 0,
	// so we return early for positive height values
	// only negative heights ("depths") are allowed
	if (height > -depthModParams[DEPTHMOD_MIN_HEIGHT]) { return 1.0f; }
	if (height < -depthModParams[DEPTHMOD_MAX_HEIGHT]) { return 0.0f; }

	const float a = depthModParams[DEPTHMOD_QUA_COEFF];
	const float b = depthModParams[DEPTHMOD_LIN_COEFF];
	const float c = depthModParams[DEPTHMOD_CON_COEFF];

	const float minScale = 0.01f;
	const float maxScale = depthModParams[DEPTHMOD_MAX_SCALE];

	const float depth = -height;
	const float scale = Clamp((a * depth * depth + b * depth + c), minScale, maxScale);

	// NOTE:
	//   <maxScale> is guaranteed to be >= 0.01, so the
	//   depth-mod range is [1.0 / 0.01, 1.0 / +infinity]
	//
	//   if minScale <= scale <       1.0, speedup
	//   if      1.0 <  scale <= maxScale, slowdown
	return (1.0f / scale);
}

unsigned int MoveData::GetCheckSum() const {
	unsigned int sum = 0;

	// NOTE:
	//   safe so long as MoveData has no virtuals and we
	//   make sure we do not checksum the pointer-members
	const unsigned char* bytes = reinterpret_cast<const unsigned char*>(this);
	const unsigned char* ptrs[3] = {
		reinterpret_cast<const unsigned char*>(&this->name),
		reinterpret_cast<const unsigned char*>(&this->moveMath),
		reinterpret_cast<const unsigned char*>(&this->tempOwner),
	};

	for (unsigned int n = 0; n < sizeof(*this); n++) {
		if (&bytes[n] == ptrs[0]) { n += sizeof(std::string);   continue; }
		if (&bytes[n] == ptrs[1]) { n += sizeof(CMoveMath*);    continue; }
		if (&bytes[n] == ptrs[2]) { n += sizeof(CSolidObject*); continue; }

		sum ^= ((n + 1) * bytes[n]);
	}

	return sum;
}

