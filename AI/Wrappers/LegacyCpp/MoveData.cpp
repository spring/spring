/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "MoveData.h"

#include "System/creg/STL_Deque.h"
#include "System/creg/STL_Map.h"


namespace springLegacyAI {

CR_BIND(MoveData, )

CR_REG_METADATA(MoveData, (
	CR_MEMBER(moveType),
	CR_MEMBER(moveFamily),
	CR_MEMBER(terrainClass),
	CR_MEMBER(followGround),

	CR_MEMBER(xsize),
	CR_MEMBER(zsize),
	CR_MEMBER(depth),
	CR_MEMBER(maxSlope),
	CR_MEMBER(slopeMod),

	CR_MEMBER(pathType),
	CR_MEMBER(crushStrength),

	CR_MEMBER(name),

	CR_MEMBER(maxSpeed),
	CR_MEMBER(maxTurnRate),

	CR_MEMBER(maxAcceleration),
	CR_MEMBER(maxBreaking),

	CR_MEMBER(subMarine)

))

} // namespace springLegacyAI

