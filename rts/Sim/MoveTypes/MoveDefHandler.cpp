/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/lexical_cast.hpp>

#include "MoveDefHandler.h"
#include "Game/Game.h"
#include "Lua/LuaParser.h"
#include "Map/ReadMap.h"
#include "Map/MapInfo.h"
#include "MoveMath/MoveMath.h"
#include "System/creg/STL_Deque.h"
#include "System/creg/STL_Map.h"
#include "System/Exceptions.h"
#include "System/CRC.h"
#include "System/myMath.h"
#include "System/Util.h"

CR_BIND(MoveDef, (0));
CR_BIND(MoveDefHandler, );

CR_REG_METADATA(MoveDef, (
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
	CR_MEMBER(speedModMults),

	CR_MEMBER(pathType),
	CR_MEMBER(unitDefRefCount),

	CR_MEMBER(followGround),
	CR_MEMBER(subMarine),

	CR_MEMBER(avoidMobilesOnPath),
	CR_MEMBER(heatMapping),
	CR_MEMBER(heatMod),
	CR_MEMBER(heatProduced),

	CR_RESERVED(16)
));

CR_REG_METADATA(MoveDefHandler, (
	CR_MEMBER(moveDefs),
	CR_MEMBER(moveDefNames),
	CR_MEMBER(checksum),
	CR_RESERVED(16)
));


MoveDefHandler* moveDefHandler;

// FIXME: do something with these magic numbers
static const float MAX_ALLOWED_WATER_DAMAGE_GMM = 1e3f;
static const float MAX_ALLOWED_WATER_DAMAGE_HMM = 1e4f;

static float DegreesToMaxSlope(float degrees)
{
	// Prevent MSVC from inlining stuff that would break the
	// PE checksum compatibility between debug and release
	static const float degToRad = PI / 180.0f;

	const float deg = Clamp(degrees, 0.0f, 60.0f) * 1.5f;
	const float rad = deg * degToRad;

	return (1.0f - math::cos(rad));
}


MoveDefHandler::MoveDefHandler()
{
	const LuaTable rootTable = game->defsParser->GetRoot().SubTable("MoveDefs");
	if (!rootTable.IsValid()) {
		throw content_error("Error loading movement definitions");
	}

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

		MoveDef* md = new MoveDef(moveTable, num);
		moveDefs.push_back(md);
		moveDefNames[md->name] = md->pathType;

		crc << md->GetCheckSum();
	}

	CMoveMath::noHoverWaterMove = (mapInfo->water.damage >= MAX_ALLOWED_WATER_DAMAGE_HMM);
	CMoveMath::waterDamageCost = (mapInfo->water.damage >= MAX_ALLOWED_WATER_DAMAGE_GMM)?
		0.0f: (1.0f / (1.0f + mapInfo->water.damage * 0.1f));

	crc << CMoveMath::waterDamageCost;
	crc << CMoveMath::noHoverWaterMove;

	checksum = crc.GetDigest();
}


MoveDefHandler::~MoveDefHandler()
{
	while (!moveDefs.empty()) {
		delete moveDefs.back();
		moveDefs.pop_back();
	}
}


MoveDef* MoveDefHandler::GetMoveDefByName(const std::string& name)
{
	map<string, int>::const_iterator it = moveDefNames.find(name);
	if (it == moveDefNames.end()) {
		return NULL;
	}
	return moveDefs[it->second];
}



MoveDef::MoveDef() {
	name              = "";

	moveType          = MoveDef::Ground_Move;
	moveFamily        = MoveDef::Tank;
	terrainClass      = MoveDef::Mixed;

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

	speedModMults[SPEEDMOD_MOBILE_BUSY_MULT] = 0.10f;
	speedModMults[SPEEDMOD_MOBILE_IDLE_MULT] = 0.35f;
	speedModMults[SPEEDMOD_MOBILE_MOVE_MULT] = 0.65f;
	speedModMults[SPEEDMOD_MOBILE_NUM_MULTS] = 0.0f;

	crushStrength     = 0.0f;

	pathType          = 0;
	unitDefRefCount   = 0;

	followGround      = true;
	subMarine         = false;

	avoidMobilesOnPath = true;

	heatMapping       = true;
	heatMod           = 0.05f;
	heatProduced      = GAME_SPEED;
}

MoveDef::MoveDef(const LuaTable& moveTable, int moveDefID) {
	*this = MoveDef();

	name          = StringToLower(moveTable.GetString("name", ""));
	pathType      = moveDefID - 1;
	crushStrength = moveTable.GetFloat("crushStrength", 10.0f);

	const LuaTable& depthModTable = moveTable.SubTable("depthModParams");
	const LuaTable& speedModMultsTable = moveTable.SubTable("speedModMults");

	const float minWaterDepth = moveTable.GetFloat("minWaterDepth", 10.0f);
	const float maxWaterDepth = moveTable.GetFloat("maxWaterDepth", 0.0f);

	if ((name.find("boat") != string::npos) ||
	    (name.find("ship") != string::npos)) {
		moveType   = MoveDef::Ship_Move;
		depth      = minWaterDepth;
		moveFamily = MoveDef::Ship;
		subMarine  = moveTable.GetBool("subMarine", false);
	} else if (name.find("hover") != string::npos) {
		moveType   = MoveDef::Hover_Move;
		maxSlope   = DegreesToMaxSlope(moveTable.GetFloat("maxSlope", 15.0f));
		moveFamily = MoveDef::Hover;
	} else {
		moveType = MoveDef::Ground_Move;
		depth    = maxWaterDepth;

		depthModParams[DEPTHMOD_MIN_HEIGHT] = std::max(0.00f, depthModTable.GetFloat("minHeight",                                     0.0f ));
		depthModParams[DEPTHMOD_MAX_HEIGHT] =         (       depthModTable.GetFloat("maxHeight",        std::numeric_limits<float>::max() ));
		depthModParams[DEPTHMOD_MAX_SCALE ] = std::max(0.01f, depthModTable.GetFloat("maxScale",         std::numeric_limits<float>::max() ));
		depthModParams[DEPTHMOD_QUA_COEFF ] = std::max(0.00f, depthModTable.GetFloat("quadraticCoeff",                                0.0f ));
		depthModParams[DEPTHMOD_LIN_COEFF ] = std::max(0.00f, depthModTable.GetFloat("linearCoeff",    moveTable.GetFloat("depthMod", 0.1f)));
		depthModParams[DEPTHMOD_CON_COEFF ] = std::max(0.00f, depthModTable.GetFloat("constantCoeff",                                 1.0f ));

		// ensure [depthModMinHeight, depthModMaxHeight] is a valid range
		depthModParams[DEPTHMOD_MAX_HEIGHT] = std::max(depthModParams[DEPTHMOD_MIN_HEIGHT], depthModParams[DEPTHMOD_MAX_HEIGHT]);

		maxSlope = DegreesToMaxSlope(moveTable.GetFloat("maxSlope", 60.0f));

		if (name.find("tank") != string::npos) {
			moveFamily = MoveDef::Tank;
		} else {
			moveFamily = MoveDef::KBot;
		}
	}

	speedModMults[SPEEDMOD_MOBILE_BUSY_MULT] = std::max(0.01f, speedModMultsTable.GetFloat("mobileBusyMult", 1.0f /*0.10f*/));
	speedModMults[SPEEDMOD_MOBILE_IDLE_MULT] = std::max(0.01f, speedModMultsTable.GetFloat("mobileIdleMult", 1.0f /*0.35f*/));
	speedModMults[SPEEDMOD_MOBILE_MOVE_MULT] = std::max(0.01f, speedModMultsTable.GetFloat("mobileMoveMult", 1.0f /*0.65f*/));

	avoidMobilesOnPath = moveTable.GetBool("avoidMobilesOnPath", true);
	heatMapping = moveTable.GetBool("heatMapping", false);
	heatMod = moveTable.GetFloat("heatMod", 50.0f);
	heatProduced = moveTable.GetInt("heatProduced", GAME_SPEED * 2);

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
		(moveFamily == MoveDef::Tank ||
		 moveFamily == MoveDef::KBot);

	// tank or bot that cannot get its threads / feet
	// wet, or hovercraft (which doesn't touch ground
	// or water)
	const bool b0 = ((followGround && maxWaterDepth <= 0.0) || moveFamily == MoveDef::Hover);

	// ship (or sub) that cannot crawl onto shore, OR tank or
	// kbot restricted to snorkling (strange but possible)
	const bool b1 = ((moveFamily == MoveDef::Ship && minWaterDepth > 0.0) || ((followGround) && minWaterDepth > 0.0));

	// tank or kbot that CAN go skinny-dipping (amph.),
	// or ship that CAN sprout legs when at the beach
	const bool b2 = ((followGround) && maxWaterDepth > 0.0) || (moveFamily == MoveDef::Ship && minWaterDepth < 0.0);

	if (b0) { terrainClass = MoveDef::Land;  }
	if (b1) { terrainClass = MoveDef::Water; }
	if (b2) { terrainClass = MoveDef::Mixed; }

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

bool MoveDef::TestMoveSquare(const int hmx, const int hmz) const {
	bool ret = true;

	// test the entire footprint
	for (int i = hmx - xsizeh; i <= hmx + xsizeh; i++) {
		for (int j = hmz - zsizeh; j <= hmz + zsizeh; j++) {
			const float speedMod = CMoveMath::GetPosSpeedMod(*this, hmx + i, hmz + j);
			const CMoveMath::BlockType blockBits = CMoveMath::IsBlocked(*this, hmx + i, hmz + j, NULL);

			// check both terrain and the blocking-map
			ret &= ((speedMod > 0.0f) && ((blockBits & CMoveMath::BLOCK_STRUCTURE) == 0));
		}
	}

	return ret;
}

float MoveDef::GetDepthMod(const float height) const {
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

unsigned int MoveDef::GetCheckSum() const {
	unsigned int sum = 0;

	const unsigned char* minByte = reinterpret_cast<const unsigned char*>(&moveType);
	const unsigned char* maxByte = reinterpret_cast<const unsigned char*>(&heatProduced) + sizeof(heatProduced);

	assert(minByte < maxByte);

	// NOTE:
	//   safe so long as MoveDef has no virtuals and we
	//   make sure we do not checksum the pointer-members
	for (const unsigned char* byte = minByte; byte != maxByte; byte++) {
		sum ^= ((((byte + 1) - minByte) << 8) * (*byte));
	}

	return sum;
}

