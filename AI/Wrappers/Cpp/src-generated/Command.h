/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_COMMAND_H
#define _CPPWRAPPER_COMMAND_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class Command {

public:
	virtual ~Command(){}
public:
	virtual int GetSkirmishAIId() const = 0;

public:
	virtual int GetCommandId() const = 0;

	/**
	 * For the type of the command queue, see CCommandQueue::CommandQueueType
	 * in Sim/Unit/CommandAI/CommandQueue.h
	 */
public:
	virtual int GetType() = 0;

	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
public:
	virtual int GetId() = 0;

public:
	virtual short GetOptions() = 0;

public:
	virtual int GetTag() = 0;

public:
	virtual int GetTimeOut() = 0;

public:
	virtual std::vector<float> GetParams() = 0;

}; // class Command

}  // namespace springai

#endif // _CPPWRAPPER_COMMAND_H

