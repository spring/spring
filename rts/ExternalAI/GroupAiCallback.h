#ifndef GROUPAICALLBACK_H
#define GROUPAICALLBACK_H
// GroupAICallback.h: interface for the CGroupAICallback class.
//
//////////////////////////////////////////////////////////////////////

#include "IGroupAiCallback.h"
#include "AICallback.h"
class CGroup;

class CGroupAICallback : public IGroupAICallback
{
public:
	CGroupAICallback(CGroup* group);
	~CGroupAICallback();

	CGroup* group;
	CAICallback aicb;

	void UpdateIcons();
	const Command* GetOrderPreview();
	bool IsSelected();
	int GetGroupId();

	IAICallback *GetAICallback ();
	int GetUnitLastUserOrder (int unitid);
};

#endif /* GROUPAICALLBACK_H */
