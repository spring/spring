/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_WRAPPORDERPREVIEW_H
#define _CPPWRAPPER_WRAPPORDERPREVIEW_H

#include <climits> // for INT_MAX (required by unit-command wrapping functions)

#include "IncludesHeaders.h"
#include "OrderPreview.h"

namespace springai {

/**
 * Lets C++ Skirmish AIs call back to the Spring engine.
 *
 * @author	AWK wrapper script
 * @version	GENERATED
 */
class WrappOrderPreview : public OrderPreview {

private:
	int skirmishAIId;

	WrappOrderPreview(int skirmishAIId);
	virtual ~WrappOrderPreview();
public:
public:
	// @Override
	virtual int GetSkirmishAIId() const;
public:
	static OrderPreview* GetInstance(int skirmishAIId);

	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
public:
	// @Override
	virtual int GetId(int groupId);

public:
	// @Override
	virtual short GetOptions(int groupId);

public:
	// @Override
	virtual int GetTag(int groupId);

public:
	// @Override
	virtual int GetTimeOut(int groupId);

public:
	// @Override
	virtual std::vector<float> GetParams(int groupId);
}; // class WrappOrderPreview

}  // namespace springai

#endif // _CPPWRAPPER_WRAPPORDERPREVIEW_H

