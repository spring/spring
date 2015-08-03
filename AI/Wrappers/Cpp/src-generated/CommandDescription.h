/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_COMMANDDESCRIPTION_H
#define _CPPWRAPPER_COMMANDDESCRIPTION_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class CommandDescription {

public:
	virtual ~CommandDescription(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetSupportedCommandId() const = 0;

	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
public:
	virtual int GetId() = 0;

public:
	virtual const char* GetName() = 0;

public:
	virtual const char* GetToolTip() = 0;

public:
	virtual bool IsShowUnique() = 0;

public:
	virtual bool IsDisabled() = 0;

public:
	virtual std::vector<const char*> GetParams() = 0;

}; // class CommandDescription

}  // namespace springai

#endif // _CPPWRAPPER_COMMANDDESCRIPTION_H

