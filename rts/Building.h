// Building.h: interface for the CBuilding class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BUILDING_H__4F3B5E65_9A35_44F1_B79E_38045D6CE26E__INCLUDED_)
#define AFX_BUILDING_H__4F3B5E65_9A35_44F1_B79E_38045D6CE26E__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Unit.h"

class CBuilding : public CUnit  
{
public:
	CBuilding(const float3 &pos,int team,UnitDef* unitDef);
	virtual ~CBuilding();

	void Init(void);
};

#endif // !defined(AFX_BUILDING_H__4F3B5E65_9A35_44F1_B79E_38045D6CE26E__INCLUDED_)
