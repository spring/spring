/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPGUI_H
#define _CPPWRAPPER_WRAPPGUI_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Gui.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappGui : public Gui {

private:
	int skirmishAIId;

	WrappGui(int skirmishAIId);
	virtual ~WrappGui();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static Gui* GetInstance(int skirmishAIId);

public:
	// @Override
	virtual float GetViewRange();

public:
	// @Override
	virtual float GetScreenX();

public:
	// @Override
	virtual float GetScreenY();

public:
	// @Override
	virtual springai::Camera* GetCamera();
}; // class WrappGui

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPGUI_H

