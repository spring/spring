#ifndef IGROUPAICALLBACK_H
#define IGROUPAICALLBACK_H
// IGroupAICallback.h: interface for the IGroupAICallback class.
// Dont modify this file
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <vector>
#include <deque>
#include "float3.h"
#include "command.h"

struct UnitDef;
struct FeatureDef;
class IAICallback;

class IGroupAICallback  
{
public:
	virtual IAICallback *GetAICallback ()=0;

	virtual void UpdateIcons()=0;														//force gui to update the icons 
	virtual const Command* GetOrderPreview()=0;									//this make the game to try to create an order from the current (unfinished) mouse state, dont count on the command being pointed to being left after you call this again or leave the function
	virtual bool IsSelected()=0;													//returns true if this group is currently selected

	virtual int GetUnitLastUserOrder(int unitid)=0;	//last frame the user gave a direct order to a unit, ai should probably leave it be for some time to avoid irritating user
};

#endif /* IGROUPAICALLBACK_H */
