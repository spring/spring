/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPFEATUREDEF_H
#define _CPPWRAPPER_WRAPPFEATUREDEF_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "FeatureDef.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappFeatureDef : public FeatureDef {

private:
	int skirmishAIId;
	int featureDefId;

	WrappFeatureDef(int skirmishAIId, int featureDefId);
	virtual ~WrappFeatureDef();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetFeatureDefId() const;
public:
	static FeatureDef* GetInstance(int skirmishAIId, int featureDefId);

public:
	// @Override
	virtual const char* GetName();

public:
	// @Override
	virtual const char* GetDescription();

public:
	// @Override
	virtual const char* GetFileName();

public:
	// @Override
	virtual float GetContainedResource(Resource* resource);

public:
	// @Override
	virtual float GetMaxHealth();

public:
	// @Override
	virtual float GetReclaimTime();

	/**
	 * Used to see if the object can be overrun by units of a certain heavyness
	 */
public:
	// @Override
	virtual float GetMass();

public:
	// @Override
	virtual bool IsUpright();

public:
	// @Override
	virtual int GetDrawType();

public:
	// @Override
	virtual const char* GetModelName();

	/**
	 * Used to determine whether the feature is resurrectable.
	 * 
	 * @return  -1: (default) only if it is the 1st wreckage of
	 *              the UnitDef it originates from
	 *           0: no, never
	 *           1: yes, always
	 */
public:
	// @Override
	virtual int GetResurrectable();

public:
	// @Override
	virtual int GetSmokeTime();

public:
	// @Override
	virtual bool IsDestructable();

public:
	// @Override
	virtual bool IsReclaimable();

public:
	// @Override
	virtual bool IsBlocking();

public:
	// @Override
	virtual bool IsBurnable();

public:
	// @Override
	virtual bool IsFloating();

public:
	// @Override
	virtual bool IsNoSelect();

public:
	// @Override
	virtual bool IsGeoThermal();

	/**
	 * Name of the FeatureDef that this turns into when killed (not reclaimed).
	 */
public:
	// @Override
	virtual const char* GetDeathFeature();

	/**
	 * Size of the feature along the X axis - in other words: height.
	 * each size is 8 units
	 */
public:
	// @Override
	virtual int GetXSize();

	/**
	 * Size of the feature along the Z axis - in other words: width.
	 * each size is 8 units
	 */
public:
	// @Override
	virtual int GetZSize();

public:
	// @Override
	virtual std::map<std::string,std::string> GetCustomParams();
}; // class WrappFeatureDef

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPFEATUREDEF_H

