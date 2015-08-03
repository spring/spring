/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_MOVEDATA_H
#define _CPPWRAPPER_MOVEDATA_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class MoveData {

public:
	virtual ~MoveData(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetUnitDefId() const = 0;

	/**
	 * @deprecated
	 */
public:
	virtual float GetMaxAcceleration() = 0;

	/**
	 * @deprecated
	 */
public:
	virtual float GetMaxBreaking() = 0;

	/**
	 * @deprecated
	 */
public:
	virtual float GetMaxSpeed() = 0;

	/**
	 * @deprecated
	 */
public:
	virtual short GetMaxTurnRate() = 0;

public:
	virtual int GetXSize() = 0;

public:
	virtual int GetZSize() = 0;

public:
	virtual float GetDepth() = 0;

public:
	virtual float GetMaxSlope() = 0;

public:
	virtual float GetSlopeMod() = 0;

public:
	virtual float GetDepthMod(float height) = 0;

public:
	virtual int GetPathType() = 0;

public:
	virtual float GetCrushStrength() = 0;

	/**
	 * enum MoveType { Ground_Move=0, Hover_Move=1, Ship_Move=2 };
	 */
public:
	virtual int GetMoveType() = 0;

	/**
	 * enum SpeedModClass { Tank=0, KBot=1, Hover=2, Ship=3 };
	 */
public:
	virtual int GetSpeedModClass() = 0;

public:
	virtual int GetTerrainClass() = 0;

public:
	virtual bool GetFollowGround() = 0;

public:
	virtual bool IsSubMarine() = 0;

public:
	virtual const char* GetName() = 0;

}; // class MoveData

}  // namespace springai

#endif // _CPPWRAPPER_MOVEDATA_H

