/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MoveDefHandler.h"
#include "Lua/LuaParser.h"
#include "Map/MapInfo.h"
#include "MoveMath/MoveMath.h"
#include "Sim/Misc/GlobalConstants.h"
#include "System/creg/STL_Map.h"
#include "System/Exceptions.h"
#include "System/CRC.h"
#include "System/SpringMath.h"
#include "System/StringHash.h"
#include "System/StringUtil.h"

CR_BIND(MoveDef, ())
CR_BIND(MoveDefHandler, )

CR_REG_METADATA(MoveDef, (
	CR_MEMBER(name),

	CR_MEMBER(speedModClass),
	CR_MEMBER(terrainClass),

	CR_MEMBER(pathType),

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

	CR_MEMBER(heatMod),
	CR_MEMBER(flowMod),
	CR_MEMBER(heatProduced),

	CR_MEMBER(followGround),
	CR_MEMBER(isSubmarine),

	CR_MEMBER(avoidMobilesOnPath),
	CR_MEMBER(allowTerrainCollisions),
	CR_MEMBER(allowRawMovement),

	CR_MEMBER(heatMapping),
	CR_MEMBER(flowMapping)
))

CR_REG_METADATA(MoveDefHandler, (
	CR_MEMBER(moveDefs),
	CR_MEMBER(nameMap),
	CR_MEMBER(mdCounter),
	CR_MEMBER(mdChecksum)
))


MoveDefHandler moveDefHandler;

// FIXME: do something with these magic numbers
static constexpr float MAX_ALLOWED_WATER_DAMAGE_GMM = 1e3f;
static constexpr float MAX_ALLOWED_WATER_DAMAGE_HMM = 1e4f;

static float DegreesToMaxSlope(float degrees)
{
	// Prevent MSVC from inlining stuff that would break the
	// PE checksum compatibility between debug and release
	static constexpr float degToRad = math::DEG_TO_RAD;

	const float deg = Clamp(degrees, 0.0f, 60.0f) * 1.5f;
	const float rad = deg * degToRad;

	return (1.0f - math::cos(rad));
}

static MoveDef::SpeedModClass ParseSpeedModClass(const std::string& moveDefName, const LuaTable& moveDefTable)
{
	const int speedModClass = moveDefTable.GetInt("speedModClass", -1);

	if (speedModClass != -1)
		return Clamp(MoveDef::SpeedModClass(speedModClass), MoveDef::Tank, MoveDef::Ship);

	// name-based fallbacks
	if (moveDefName.find( "boat") != string::npos)
		return MoveDef::Ship;
	if (moveDefName.find( "ship") != string::npos)
		return MoveDef::Ship;
	if (moveDefName.find("hover") != string::npos)
		return MoveDef::Hover;
	if (moveDefName.find( "tank") != string::npos)
		return MoveDef::Tank;

	return MoveDef::KBot;
}



void MoveDefHandler::Init(LuaParser* defsParser)
{
	const LuaTable& rootTable = defsParser->GetRoot().SubTable("MoveDefs");

	if (!rootTable.IsValid())
		throw content_error("[MoveDefHandler] error loading MoveDef entries");
	if (rootTable.GetLength() > moveDefs.size())
		throw content_error("[MoveDefHandler] too many MoveDef entries");

	CRC crc;

	for (const CMapInfo::TerrainType& terrType: mapInfo->terrainTypes) {
		crc << terrType.tankSpeed << terrType.kbotSpeed;
		crc << terrType.hoverSpeed << terrType.shipSpeed;
	}


	nameMap.clear();
	nameMap.reserve(rootTable.GetLength());

	for (unsigned int moveDefID = 0; /* no test */; moveDefID++) {
		const LuaTable& moveDefTable = rootTable.SubTable(moveDefID + 1);

		if (!moveDefTable.IsValid())
			break;

		moveDefs[mdCounter] = {moveDefTable};
		nameMap[hashString(moveDefs[mdCounter].name.c_str())] = (moveDefs[mdCounter].pathType = moveDefID);

		crc << moveDefs[mdCounter++].CalcCheckSum();
	}

	CMoveMath::noHoverWaterMove = (mapInfo->water.damage >= MAX_ALLOWED_WATER_DAMAGE_HMM);
	CMoveMath::waterDamageCost = (mapInfo->water.damage >= MAX_ALLOWED_WATER_DAMAGE_GMM)?
		0.0f: (1.0f / (1.0f + mapInfo->water.damage * 0.1f));

	crc << CMoveMath::waterDamageCost;
	crc << CMoveMath::noHoverWaterMove;

	mdChecksum = crc.GetDigest();
}


MoveDef* MoveDefHandler::GetMoveDefByName(const std::string& name)
{
	const auto it = nameMap.find(hashString(name.c_str()));

	if (it == nameMap.end())
		return nullptr;

	return &moveDefs[it->second];
}



MoveDef::MoveDef()
{
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
}

MoveDef::MoveDef(const LuaTable& moveDefTable): MoveDef() {
	name          = StringToLower(moveDefTable.GetString("name", ""));
	crushStrength = moveDefTable.GetFloat("crushStrength", 10.0f);

	const LuaTable& depthModTable = moveDefTable.SubTable("depthModParams");
	const LuaTable& speedModMultsTable = moveDefTable.SubTable("speedModMults");

	const float minWaterDepth = moveDefTable.GetFloat("minWaterDepth", GetDefaultMinWaterDepth());
	const float maxWaterDepth = moveDefTable.GetFloat("maxWaterDepth", GetDefaultMaxWaterDepth());

	switch ((speedModClass = ParseSpeedModClass(name, moveDefTable))) {
		case MoveDef::Tank: // fall-through
		case MoveDef::KBot: {
			depthModParams[DEPTHMOD_MIN_HEIGHT] = std::max(0.00f, depthModTable.GetFloat("minHeight",                                        0.0f ));
			depthModParams[DEPTHMOD_MAX_HEIGHT] =         (       depthModTable.GetFloat("maxHeight",           std::numeric_limits<float>::max() ));
			depthModParams[DEPTHMOD_MAX_SCALE ] = std::max(0.01f, depthModTable.GetFloat("maxScale",            std::numeric_limits<float>::max() ));
			depthModParams[DEPTHMOD_QUA_COEFF ] = std::max(0.00f, depthModTable.GetFloat("quadraticCoeff",                                   0.0f ));
			depthModParams[DEPTHMOD_LIN_COEFF ] = std::max(0.00f, depthModTable.GetFloat("linearCoeff",    moveDefTable.GetFloat("depthMod", 0.1f)));
			depthModParams[DEPTHMOD_CON_COEFF ] = std::max(0.00f, depthModTable.GetFloat("constantCoeff",                                    1.0f ));

			// ensure [depthModMinHeight, depthModMaxHeight] is a valid range
			depthModParams[DEPTHMOD_MAX_HEIGHT] = std::max(depthModParams[DEPTHMOD_MIN_HEIGHT], depthModParams[DEPTHMOD_MAX_HEIGHT]);

			depth    = maxWaterDepth;
			maxSlope = DegreesToMaxSlope(moveDefTable.GetFloat("maxSlope", 60.0f));
		} break;

		case MoveDef::Hover: {
			depth    = maxWaterDepth;
			maxSlope = DegreesToMaxSlope(moveDefTable.GetFloat("maxSlope", 15.0f));
		} break;

		case MoveDef::Ship: {
			depth    = minWaterDepth;
			// maxSlope = "n/a";

			isSubmarine = moveDefTable.GetBool("subMarine", false);
		} break;
	}

	speedModMults[SPEEDMOD_MOBILE_BUSY_MULT] = std::max(0.01f, speedModMultsTable.GetFloat("mobileBusyMult", 1.0f /*0.10f*/));
	speedModMults[SPEEDMOD_MOBILE_IDLE_MULT] = std::max(0.01f, speedModMultsTable.GetFloat("mobileIdleMult", 1.0f /*0.35f*/));
	speedModMults[SPEEDMOD_MOBILE_MOVE_MULT] = std::max(0.01f, speedModMultsTable.GetFloat("mobileMoveMult", 1.0f /*0.65f*/));

	avoidMobilesOnPath = moveDefTable.GetBool("avoidMobilesOnPath", true);
	allowTerrainCollisions = moveDefTable.GetBool("allowTerrainCollisions", true);
	allowRawMovement = moveDefTable.GetBool("allowRawMovement", false);

	heatMapping = moveDefTable.GetBool("heatMapping", false);
	flowMapping = moveDefTable.GetBool("flowMapping", true);

	heatMod = moveDefTable.GetFloat("heatMod", (1.0f / (GAME_SPEED * 2)) * 0.25f);
	flowMod = moveDefTable.GetFloat("flowMod", 1.0f);

	// by default heat decays to zero after N=2 seconds
	//
	// the cost contribution to a square from heat must
	// be on the same order as its normal movement cost
	// PER FRAME, i.e. such that heatMod * heatProduced
	// ~= O(1 / (GAME_SPEED * N)) because unit behavior
	// in groups quickly becomes FUBAR if heatMod >>> 1
	//
	heatProduced = moveDefTable.GetInt("heatProduced", GAME_SPEED * 2);

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
	slopeMod = moveDefTable.GetFloat("slopeMod", 4.0f / (maxSlope + 0.001f));

	// ground units hug the ocean floor when in water,
	// ships stay at a "fixed" level (their waterline)
	followGround = (speedModClass == MoveDef::Tank || speedModClass == MoveDef::KBot);

	// TODO:
	//   remove terrainClass, not used anywhere
	//   and only AI's MIGHT have benefit from it
	//
	// tank or bot that cannot get its threads / feet
	// wet, or hovercraft (which doesn't touch ground
	// or water)
	if ((followGround && maxWaterDepth <= 0.0f) || speedModClass == MoveDef::Hover)
		terrainClass = MoveDef::Land;
	// ship (or sub) that cannot crawl onto shore, OR tank
	// or bot restricted to snorkling (strange but possible)
	if ((speedModClass == MoveDef::Ship && minWaterDepth > 0.0f) || (followGround && minWaterDepth > 0.0f))
		terrainClass = MoveDef::Water;
	// tank or kbot that CAN go skinny-dipping (amph.),
	// or ship that CAN sprout legs when at the beach
	if ((followGround && maxWaterDepth > 0.0f) || (speedModClass == MoveDef::Ship && minWaterDepth < 0.0f))
		terrainClass = MoveDef::Mixed;

	const int xsizeDef = std::max(1, moveDefTable.GetInt("footprintX",        1));
	const int zsizeDef = std::max(1, moveDefTable.GetInt("footprintZ", xsizeDef));

	// make all mobile footprints point-symmetric in heightmap space
	// (meaning that only non-even dimensions are possible and each
	// footprint always has a unique center square)
	xsize = xsizeDef * SPRING_FOOTPRINT_SCALE;
	zsize = zsizeDef * SPRING_FOOTPRINT_SCALE;
	xsize -= ((xsize & 1)? 0: 1);
	zsize -= ((zsize & 1)? 0: 1);
	// precalculated data for MoveMath
	xsizeh = xsize >> 1;
	zsizeh = zsize >> 1;
	assert((xsize & 1) == 1);
	assert((zsize & 1) == 1);
}


bool MoveDef::TestMoveSquareRange(
	const CSolidObject* collider,
	const float3 rangeMins,
	const float3 rangeMaxs,
	const float3 testMoveDir,
	bool testTerrain,
	bool testObjects,
	bool centerOnly,
	float* minSpeedModPtr,
	int* maxBlockBitPtr
) const {
	assert(testTerrain || testObjects);

	const int xmin = int(rangeMins.x / SQUARE_SIZE) - xsizeh * (1 - centerOnly);
	const int zmin = int(rangeMins.z / SQUARE_SIZE) - zsizeh * (1 - centerOnly);
	const int xmax = int(rangeMaxs.x / SQUARE_SIZE) + xsizeh * (1 - centerOnly);
	const int zmax = int(rangeMaxs.z / SQUARE_SIZE) + zsizeh * (1 - centerOnly);

	const float3 testMoveDir2D = (testMoveDir * XZVector).SafeNormalize2D();

	float minSpeedMod = std::numeric_limits<float>::max();
	int   maxBlockBit = CMoveMath::BLOCK_NONE;

	bool retTestMove = true;

	for (int z = zmin; retTestMove && z <= zmax; z += 1) {
		for (int x = xmin; retTestMove && x <= xmax; x += 1) {
			const float speedMod = CMoveMath::GetPosSpeedMod(*this, x, z, testMoveDir2D);

			minSpeedMod  = std::min(minSpeedMod, speedMod);
			retTestMove &= (!testTerrain || (speedMod > 0.0f));
		}
	}

	// GetPosSpeedMod only checks *one* square of terrain
	// (heightmap/slopemap/typemap), not the blocking-map
	if (retTestMove) {
		const CMoveMath::BlockType blockBits = CMoveMath::RangeIsBlocked(*this, xmin, xmax, zmin, zmax, collider);

		maxBlockBit |= blockBits;
		retTestMove &= (!testObjects || (blockBits & CMoveMath::BLOCK_STRUCTURE) == 0);
	}

	// don't use std::min or |= because the ptr values might be garbage
	if (minSpeedModPtr != nullptr) *minSpeedModPtr = minSpeedMod;
	if (maxBlockBitPtr != nullptr) *maxBlockBitPtr = maxBlockBit;
	return retTestMove;
}



float MoveDef::CalcFootPrintMinExteriorRadius(float scale) const { return ((math::sqrt((xsize * xsize + zsize * zsize)) * 0.5f * SQUARE_SIZE) * scale); }
float MoveDef::CalcFootPrintMaxInteriorRadius(float scale) const { return ((std::max(xsize, zsize) * 0.5f * SQUARE_SIZE) * scale); }
float MoveDef::CalcFootPrintAxisStretchFactor() const
{
	return (std::abs(xsize - zsize) * 1.0f / (xsize + zsize));
}


float MoveDef::GetDepthMod(float height) const {
	// [DEPTHMOD_{MIN, MAX}_HEIGHT] are always >= 0,
	// so we return early for positive height values
	// only negative heights ("depths") are allowed
	if (height > -depthModParams[DEPTHMOD_MIN_HEIGHT])
		return 1.0f;
	if (height < -depthModParams[DEPTHMOD_MAX_HEIGHT])
		return 0.0f;

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

unsigned int MoveDef::CalcCheckSum() const {
	unsigned int sum = 0;

	const unsigned char* minByte = reinterpret_cast<const unsigned char*>(&speedModClass);
	const unsigned char* maxByte = reinterpret_cast<const unsigned char*>(&flowMapping) + sizeof(flowMapping);

	assert(minByte < maxByte);

	// NOTE:
	//   safe so long as MoveDef has no virtuals and we
	//   make sure we do not checksum any padding bytes
	for (const unsigned char* byte = minByte; byte != maxByte; byte++) {
		sum ^= ((((byte + 1) - minByte) << 8) * (*byte));
	}

	return sum;
}

