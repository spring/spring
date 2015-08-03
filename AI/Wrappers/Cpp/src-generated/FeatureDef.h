/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_FEATUREDEF_H
#define _CPPWRAPPER_FEATUREDEF_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class FeatureDef {

public:
	virtual ~FeatureDef(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetFeatureDefId() const = 0;

public:
	virtual const char* GetName() = 0;

public:
	virtual const char* GetDescription() = 0;

public:
	virtual const char* GetFileName() = 0;

public:
	virtual float GetContainedResource(Resource* resource) = 0;

public:
	virtual float GetMaxHealth() = 0;

public:
	virtual float GetReclaimTime() = 0;

	/**
	 * Used to see if the object can be overrun by units of a certain heavyness
	 */
public:
	virtual float GetMass() = 0;

public:
	virtual bool IsUpright() = 0;

public:
	virtual int GetDrawType() = 0;

public:
	virtual const char* GetModelName() = 0;

	/**
	 * Used to determine whether the feature is resurrectable.
	 * 
	 * @return  -1: (default) only if it is the 1st wreckage of
	 *              the UnitDef it originates from
	 *           0: no, never
	 *           1: yes, always
	 */
public:
	virtual int GetResurrectable() = 0;

public:
	virtual int GetSmokeTime() = 0;

public:
	virtual bool IsDestructable() = 0;

public:
	virtual bool IsReclaimable() = 0;

public:
	virtual bool IsBlocking() = 0;

public:
	virtual bool IsBurnable() = 0;

public:
	virtual bool IsFloating() = 0;

public:
	virtual bool IsNoSelect() = 0;

public:
	virtual bool IsGeoThermal() = 0;

	/**
	 * Name of the FeatureDef that this turns into when killed (not reclaimed).
	 */
public:
	virtual const char* GetDeathFeature() = 0;

	/**
	 * Size of the feature along the X axis - in other words: height.
	 * each size is 8 units
	 */
public:
	virtual int GetXSize() = 0;

	/**
	 * Size of the feature along the Z axis - in other words: width.
	 * each size is 8 units
	 */
public:
	virtual int GetZSize() = 0;

public:
	virtual std::map<std::string,std::string> GetCustomParams() = 0;

}; // class FeatureDef

}  // namespace springai

#endif // _CPPWRAPPER_FEATUREDEF_H

