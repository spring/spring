/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#ifndef _CPPWRAPPER_STUBORDERPREVIEW_H
#define _CPPWRAPPER_STUBORDERPREVIEW_H

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
class StubOrderPreview : public OrderPreview {

protected:
	virtual ~StubOrderPreview();
private:
	int skirmishAIId;/* = 0;*/ // TODO: FIXME: put this into a constructor
public:
	virtual void SetSkirmishAIId(int skirmishAIId);
	// @Override
	virtual int GetSkirmishAIId() const;
	/**
	 * For the id, see CMD_xxx codes in Sim/Unit/CommandAI/Command.h
	 * (custom codes can also be used)
	 */
	// @Override
	virtual int GetId(int groupId);
	// @Override
	virtual short GetOptions(int groupId);
	// @Override
	virtual int GetTag(int groupId);
	// @Override
	virtual int GetTimeOut(int groupId);
	// @Override
	virtual std::vector<float> GetParams(int groupId);
}; // class StubOrderPreview

}  // namespace springai

#endif // _CPPWRAPPER_STUBORDERPREVIEW_H

