/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_ORDERPREVIEW_H
#define _CPPWRAPPER_ORDERPREVIEW_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class OrderPreview {

public:
	virtual ~OrderPreview(){}
public:
	virtual int GetSkirmishAIId() const = 0;

	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
public:
	virtual int GetId(int groupId) = 0;

public:
	virtual short GetOptions(int groupId) = 0;

public:
	virtual int GetTag(int groupId) = 0;

public:
	virtual int GetTimeOut(int groupId) = 0;

public:
	virtual std::vector<float> GetParams(int groupId) = 0;

}; // class OrderPreview

}  // namespace springai

#endif // _CPPWRAPPER_ORDERPREVIEW_H

