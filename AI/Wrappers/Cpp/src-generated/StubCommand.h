/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBCOMMAND_H
#define _CPPWRAPPER_STUBCOMMAND_H

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
class StubCommand : public Command {

protected:
	virtual ~StubCommand();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int commandId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetCommandId(int commandId);
	// @Override
	virtual int GetCommandId() const;
	/**
	 * For the type of the command queue, see CCommandQueue::CommandQueueType
	 * in Sim/Unit/CommandAI/CommandQueue.h
	 */
private:
	int type;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetType(int type);
	// @Override
	virtual int GetType();
	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
private:
	int id;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetId(int id);
	// @Override
	virtual int GetId();
private:
	short options;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetOptions(short options);
	// @Override
	virtual short GetOptions();
private:
	int tag;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTag(int tag);
	// @Override
	virtual int GetTag();
private:
	int timeOut;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetTimeOut(int timeOut);
	// @Override
	virtual int GetTimeOut();
private:
	std::vector<float> params;/* = std::vector<float>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetParams(std::vector<float> params);
	// @Override
	virtual std::vector<float> GetParams();
}; // class StubCommand

}  // namespace springai

#endif // _CPPWRAPPER_STUBCOMMAND_H

