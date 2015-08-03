/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPCAMERA_H
#define _CPPWRAPPER_WRAPPCAMERA_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Camera.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappCamera : public Camera {

private:
	int skirmishAIId;

	WrappCamera(int skirmishAIId);
	virtual ~WrappCamera();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static Camera* GetInstance(int skirmishAIId);

public:
	// @Override
	virtual springai::AIFloat3 GetDirection();

public:
	// @Override
	virtual springai::AIFloat3 GetPosition();
}; // class WrappCamera

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPCAMERA_H

