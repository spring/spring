/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBCOMMANDDESCRIPTION_H
#define _CPPWRAPPER_STUBCOMMANDDESCRIPTION_H

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
class StubCommandDescription : public CommandDescription {

protected:
	virtual ~StubCommandDescription();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
private:
	int supportedCommandId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSupportedCommandId(int supportedCommandId);
	// @Override
	virtual int GetSupportedCommandId() const;
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
	const char* name;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetName(const char* name);
	// @Override
	virtual const char* GetName();
private:
	const char* toolTip;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetToolTip(const char* toolTip);
	// @Override
	virtual const char* GetToolTip();
private:
	bool isShowUnique;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetShowUnique(bool isShowUnique);
	// @Override
	virtual bool IsShowUnique();
private:
	bool isDisabled;/* = false;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetDisabled(bool isDisabled);
	// @Override
	virtual bool IsDisabled();
private:
	std::vector<const char*> params;/* = std::vector<const char*>();*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetParams(std::vector<const char*> params);
	// @Override
	virtual std::vector<const char*> GetParams();
}; // class StubCommandDescription

}  // namespace springai

#endif // _CPPWRAPPER_STUBCOMMANDDESCRIPTION_H

