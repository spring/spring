/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPFEATURE_H
#define _CPPWRAPPER_WRAPPFEATURE_H

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
class WrappFeature : public Feature {

private:
	int skirmishAIId;
	int featureId;

	WrappFeature(int skirmishAIId, int featureId);
	virtual ~WrappFeature();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	// @Override
	virtual int GetFeatureId() const;
public:
	static Feature* GetInstance(int skirmishAIId, int featureId);

public:
	// @Override
	virtual springai::FeatureDef* GetDef();

public:
	// @Override
	virtual float GetHealth();

public:
	// @Override
	virtual float GetReclaimLeft();

public:
	// @Override
	virtual springai::AIFloat3 GetPosition();
}; // class WrappFeature

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPFEATURE_H

