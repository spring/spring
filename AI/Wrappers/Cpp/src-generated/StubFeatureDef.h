/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBFEATUREDEF_H
#define _CPPWRAPPER_STUBFEATUREDEF_H

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
class StubFeatureDef : public FeatureDef {

protected:
	virtual ~StubFeatureDef();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int featureDefId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFeatureDefId(int featureDefId);
	// @Override
	virtual int GetFeatureDefId() const;
private:
	const char* name;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetName(const char* name);
	// @Override
	virtual const char* GetName();
private:
	const char* description;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDescription(const char* description);
	// @Override
	virtual const char* GetDescription();
private:
	const char* fileName;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFileName(const char* fileName);
	// @Override
	virtual const char* GetFileName();
	// @Override
	virtual float GetContainedResource(Resource* resource);
private:
	float maxHealth;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMaxHealth(float maxHealth);
	// @Override
	virtual float GetMaxHealth();
private:
	float reclaimTime;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetReclaimTime(float reclaimTime);
	// @Override
	virtual float GetReclaimTime();
	/**
	 * Used to see if the object can be overrun by units of a certain heavyness
	 */
private:
	float mass;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetMass(float mass);
	// @Override
	virtual float GetMass();
private:
	bool isUpright;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetUpright(bool isUpright);
	// @Override
	virtual bool IsUpright();
private:
	int drawType;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDrawType(int drawType);
	// @Override
	virtual int GetDrawType();
private:
	const char* modelName;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetModelName(const char* modelName);
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
private:
	int resurrectable;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetResurrectable(int resurrectable);
	// @Override
	virtual int GetResurrectable();
private:
	int smokeTime;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSmokeTime(int smokeTime);
	// @Override
	virtual int GetSmokeTime();
private:
	bool isDestructable;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDestructable(bool isDestructable);
	// @Override
	virtual bool IsDestructable();
private:
	bool isReclaimable;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetReclaimable(bool isReclaimable);
	// @Override
	virtual bool IsReclaimable();
private:
	bool isBlocking;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBlocking(bool isBlocking);
	// @Override
	virtual bool IsBlocking();
private:
	bool isBurnable;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetBurnable(bool isBurnable);
	// @Override
	virtual bool IsBurnable();
private:
	bool isFloating;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFloating(bool isFloating);
	// @Override
	virtual bool IsFloating();
private:
	bool isNoSelect;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetNoSelect(bool isNoSelect);
	// @Override
	virtual bool IsNoSelect();
private:
	bool isGeoThermal;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetGeoThermal(bool isGeoThermal);
	// @Override
	virtual bool IsGeoThermal();
	/**
	 * Name of the FeatureDef that this turns into when killed (not reclaimed).
	 */
private:
	const char* deathFeature;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDeathFeature(const char* deathFeature);
	// @Override
	virtual const char* GetDeathFeature();
	/**
	 * Size of the feature along the X axis - in other words: height.
	 * each size is 8 units
	 */
private:
	int xSize;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetXSize(int xSize);
	// @Override
	virtual int GetXSize();
	/**
	 * Size of the feature along the Z axis - in other words: width.
	 * each size is 8 units
	 */
private:
	int zSize;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetZSize(int zSize);
	// @Override
	virtual int GetZSize();
private:
	std::map<std::string,std::string> customParams;/* = std::map<std::string,std::string>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCustomParams(std::map<std::string,std::string> customParams);
	// @Override
	virtual std::map<std::string,std::string> GetCustomParams();
}; // class StubFeatureDef

}  // namespace springai

#endif // _CPPWRAPPER_STUBFEATUREDEF_H

