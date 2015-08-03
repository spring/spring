/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBCAMERA_H
#define _CPPWRAPPER_STUBCAMERA_H

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
class StubCamera : public Camera {

protected:
	virtual ~StubCamera();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	springai::AIFloat3 direction;/* = springai::AIFloat3::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDirection(springai::AIFloat3 direction);
	// @Override
	virtual springai::AIFloat3 GetDirection();
private:
	springai::AIFloat3 position;/* = springai::AIFloat3::NULL_VALUE;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetPosition(springai::AIFloat3 position);
	// @Override
	virtual springai::AIFloat3 GetPosition();
}; // class StubCamera

}  // namespace springai

#endif // _CPPWRAPPER_STUBCAMERA_H

