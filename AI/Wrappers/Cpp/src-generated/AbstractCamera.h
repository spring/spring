/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_ABSTRACTCAMERA_H
#define _CPPWRAPPER_ABSTRACTCAMERA_H

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
class AbstractCamera : public Camera {

protected:
	virtual ~AbstractCamera();
	// @Override
public:
	virtual int CompareTo(const Camera& other);
	// @Override
public:
	virtual bool Equals(const Camera& other);
	// @Override
public:
	virtual int HashCode();
	// @Override
public:
	virtual std::string ToString();
}; // class AbstractCamera

}  // namespace springai

#endif // _CPPWRAPPER_ABSTRACTCAMERA_H

