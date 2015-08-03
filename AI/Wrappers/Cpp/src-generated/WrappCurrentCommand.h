/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPCURRENTCOMMAND_H
#define _CPPWRAPPER_WRAPPCURRENTCOMMAND_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "Command.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappCurrentCommand : public Command {

private:
	int skirmishAIId;
	int unitId;
	int commandId;

	WrappCurrentCommand(int skirmishAIId, int unitId, int commandId);
	virtual ~WrappCurrentCommand();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	virtual int GetUnitId() const;
public:
	// @Override
	virtual int GetCommandId() const;
public:
	static Command* GetInstance(int skirmishAIId, int unitId, int commandId);

	/**
	 * For the type of the command queue, see CCommandQueue::CommandQueueType
	 * in Sim/Unit/CommandAI/CommandQueue.h
	 */
public:
	// @Override
	virtual int GetType();

	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
public:
	// @Override
	virtual int GetId();

public:
	// @Override
	virtual short GetOptions();

public:
	// @Override
	virtual int GetTag();

public:
	// @Override
	virtual int GetTimeOut();

public:
	// @Override
	virtual std::vector<float> GetParams();
}; // class WrappCurrentCommand

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPCURRENTCOMMAND_H

