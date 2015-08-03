/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPUNITSUPPORTEDCOMMAND_H
#define _CPPWRAPPER_WRAPPUNITSUPPORTEDCOMMAND_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "CommandDescription.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappUnitSupportedCommand : public CommandDescription {

private:
	int skirmishAIId;
	int unitId;
	int supportedCommandId;

	WrappUnitSupportedCommand(int skirmishAIId, int unitId, int supportedCommandId);
	virtual ~WrappUnitSupportedCommand();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	virtual int GetUnitId() const;
public:
	// @Override
	virtual int GetSupportedCommandId() const;
public:
	static CommandDescription* GetInstance(int skirmishAIId, int unitId, int supportedCommandId);

	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
public:
	// @Override
	virtual int GetId();

public:
	// @Override
	virtual const char* GetName();

public:
	// @Override
	virtual const char* GetToolTip();

public:
	// @Override
	virtual bool IsShowUnique();

public:
	// @Override
	virtual bool IsDisabled();

public:
	// @Override
	virtual std::vector<const char*> GetParams();
}; // class WrappUnitSupportedCommand

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPUNITSUPPORTEDCOMMAND_H

