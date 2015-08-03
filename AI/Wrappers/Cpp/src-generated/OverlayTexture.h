/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_OVERLAYTEXTURE_H
#define _CPPWRAPPER_OVERLAYTEXTURE_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class OverlayTexture {

public:
	virtual ~OverlayTexture(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetOverlayTextureId() const = 0;

public:
	virtual void Update(const float* texData, int x, int y, int w, int h) = 0;

public:
	virtual void Remove() = 0;

public:
	virtual void SetPosition(float x, float y) = 0;

public:
	virtual void SetSize(float w, float h) = 0;

public:
	virtual void SetLabel(const char* label) = 0;

}; // class OverlayTexture

}  // namespace springai

#endif // _CPPWRAPPER_OVERLAYTEXTURE_H

