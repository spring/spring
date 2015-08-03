/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPMOVEDATA_H
#define _CPPWRAPPER_WRAPPMOVEDATA_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "MoveData.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappMoveData : public MoveData {

private:
	int skirmishAIId;
	int unitDefId;

	WrappMoveData(int skirmishAIId, int unitDefId);
	virtual ~WrappMoveData();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetUnitDefId() const;
public:
	static MoveData* GetInstance(int skirmishAIId, int unitDefId);

	/**
	 * @deprecated
	 */
public:
	// @Override
	virtual float GetMaxAcceleration();

	/**
	 * @deprecated
	 */
public:
	// @Override
	virtual float GetMaxBreaking();

	/**
	 * @deprecated
	 */
public:
	// @Override
	virtual float GetMaxSpeed();

	/**
	 * @deprecated
	 */
public:
	// @Override
	virtual short GetMaxTurnRate();

public:
	// @Override
	virtual int GetXSize();

public:
	// @Override
	virtual int GetZSize();

public:
	// @Override
	virtual float GetDepth();

public:
	// @Override
	virtual float GetMaxSlope();

public:
	// @Override
	virtual float GetSlopeMod();

public:
	// @Override
	virtual float GetDepthMod(float height);

public:
	// @Override
	virtual int GetPathType();

public:
	// @Override
	virtual float GetCrushStrength();

	/**
	 * enum MoveType { Ground_Move=0, Hover_Move=1, Ship_Move=2 };
	 */
public:
	// @Override
	virtual int GetMoveType();

	/**
	 * enum SpeedModClass { Tank=0, KBot=1, Hover=2, Ship=3 };
	 */
public:
	// @Override
	virtual int GetSpeedModClass();

public:
	// @Override
	virtual int GetTerrainClass();

public:
	// @Override
	virtual bool GetFollowGround();

public:
	// @Override
	virtual bool IsSubMarine();

public:
	// @Override
	virtual const char* GetName();
}; // class WrappMoveData

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPMOVEDATA_H

