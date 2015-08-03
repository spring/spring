/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_GUI_H
#define _CPPWRAPPER_GUI_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Gui {

public:
	virtual ~Gui(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual float GetViewRange() = 0;

public:
	virtual float GetScreenX() = 0;

public:
	virtual float GetScreenY() = 0;

public:
	virtual springai::Camera* GetCamera() = 0;

}; // class Gui

}  // namespace springai

#endif // _CPPWRAPPER_GUI_H

