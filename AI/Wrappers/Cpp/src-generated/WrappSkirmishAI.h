/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPSKIRMISHAI_H
#define _CPPWRAPPER_WRAPPSKIRMISHAI_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "SkirmishAI.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappSkirmishAI : public SkirmishAI {

private:
	int skirmishAIId;

	WrappSkirmishAI(int skirmishAIId);
	virtual ~WrappSkirmishAI();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static SkirmishAI* GetInstance(int skirmishAIId);

	/**
	 * Returns the ID of the team controled by this Skirmish AI.
	 */
public:
	// @Override
	virtual int GetTeamId();

public:
	// @Override
	virtual springai::OptionValues* GetOptionValues();

public:
	// @Override
	virtual springai::Info* GetInfo();
}; // class WrappSkirmishAI

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPSKIRMISHAI_H

