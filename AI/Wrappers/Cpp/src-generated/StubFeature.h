/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBFEATURE_H
#define _CPPWRAPPER_STUBFEATURE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Feature.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class StubFeature : public Feature {

protected:
	virtual ~StubFeature();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int featureId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetFeatureId(int featureId);
	// @Override
	virtual int GetFeatureId() const;
private:
	springai::FeatureDef* def;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDef(springai::FeatureDef* def);
	// @Override
	virtual springai::FeatureDef* GetDef();
private:
	float health;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetHealth(float health);
	// @Override
	virtual float GetHealth();
private:
	float reclaimLeft;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetReclaimLeft(float reclaimLeft);
	// @Override
	virtual float GetReclaimLeft();
private:
	springai::AIFloat3 position;/* = springai::AIFloat3::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPosition(springai::AIFloat3 position);
	// @Override
	virtual springai::AIFloat3 GetPosition();
}; // class StubFeature

}  // namespace springai

#endif // _CPPWRAPPER_STUBFEATURE_H

