/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBSKIRMISHAI_H
#define _CPPWRAPPER_STUBSKIRMISHAI_H

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
class StubSkirmishAI : public SkirmishAI {

protected:
	virtual ~StubSkirmishAI();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	/**
	 * Returns the ID of the team controled by this Skirmish AI.
	 */
private:
	int teamId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTeamId(int teamId);
	// @Override
	virtual int GetTeamId();
private:
	springai::OptionValues* optionValues;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetOptionValues(springai::OptionValues* optionValues);
	// @Override
	virtual springai::OptionValues* GetOptionValues();
private:
	springai::Info* info;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetInfo(springai::Info* info);
	// @Override
	virtual springai::Info* GetInfo();
}; // class StubSkirmishAI

}  // namespace springai

#endif // _CPPWRAPPER_STUBSKIRMISHAI_H

