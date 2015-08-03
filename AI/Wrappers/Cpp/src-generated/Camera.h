/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_CAMERA_H
#define _CPPWRAPPER_CAMERA_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Camera {

public:
	virtual ~Camera(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual springai::AIFloat3 GetDirection() = 0;

public:
	virtual springai::AIFloat3 GetPosition() = 0;

}; // class Camera

}  // namespace springai

#endif // _CPPWRAPPER_CAMERA_H

