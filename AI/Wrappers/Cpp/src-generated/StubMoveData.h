/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBMOVEDATA_H
#define _CPPWRAPPER_STUBMOVEDATA_H

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
class StubMoveData : public MoveData {

protected:
	virtual ~StubMoveData();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int unitDefId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUnitDefId(int unitDefId);
	// @Override
	virtual int GetUnitDefId() const;
	/**
	 * @deprecated
	 */
private:
	float maxAcceleration;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxAcceleration(float maxAcceleration);
	// @Override
	virtual float GetMaxAcceleration();
	/**
	 * @deprecated
	 */
private:
	float maxBreaking;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxBreaking(float maxBreaking);
	// @Override
	virtual float GetMaxBreaking();
	/**
	 * @deprecated
	 */
private:
	float maxSpeed;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxSpeed(float maxSpeed);
	// @Override
	virtual float GetMaxSpeed();
	/**
	 * @deprecated
	 */
private:
	short maxTurnRate;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxTurnRate(short maxTurnRate);
	// @Override
	virtual short GetMaxTurnRate();
private:
	int xSize;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetXSize(int xSize);
	// @Override
	virtual int GetXSize();
private:
	int zSize;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetZSize(int zSize);
	// @Override
	virtual int GetZSize();
private:
	float depth;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDepth(float depth);
	// @Override
	virtual float GetDepth();
private:
	float maxSlope;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxSlope(float maxSlope);
	// @Override
	virtual float GetMaxSlope();
private:
	float slopeMod;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSlopeMod(float slopeMod);
	// @Override
	virtual float GetSlopeMod();
	// @Override
	virtual float GetDepthMod(float height);
private:
	int pathType;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPathType(int pathType);
	// @Override
	virtual int GetPathType();
private:
	float crushStrength;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCrushStrength(float crushStrength);
	// @Override
	virtual float GetCrushStrength();
	/**
	 * enum MoveType { Ground_Move=0, Hover_Move=1, Ship_Move=2 };
	 */
private:
	int moveType;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMoveType(int moveType);
	// @Override
	virtual int GetMoveType();
	/**
	 * enum SpeedModClass { Tank=0, KBot=1, Hover=2, Ship=3 };
	 */
private:
	int speedModClass;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSpeedModClass(int speedModClass);
	// @Override
	virtual int GetSpeedModClass();
private:
	int terrainClass;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTerrainClass(int terrainClass);
	// @Override
	virtual int GetTerrainClass();
private:
	bool followGround;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFollowGround(bool followGround);
	// @Override
	virtual bool GetFollowGround();
private:
	bool isSubMarine;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSubMarine(bool isSubMarine);
	// @Override
	virtual bool IsSubMarine();
private:
	const char* name;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetName(const char* name);
	// @Override
	virtual const char* GetName();
}; // class StubMoveData

}  // namespace springai

#endif // _CPPWRAPPER_STUBMOVEDATA_H

