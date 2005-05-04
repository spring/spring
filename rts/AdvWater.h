// DrawWater.h: interface for the CDrawWater class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ADVWATER_H__B59D3FE0_03FC_4A16_AEE4_6384895BD3AE__INCLUDED_)
#define AFX_ADVWATER_H__B59D3FE0_03FC_4A16_AEE4_6384895BD3AE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "basewater.h"

class CAdvWater : public CBaseWater
{
public:
	void UpdateWater(CGame* game);
	void Draw();
	CAdvWater();
	virtual ~CAdvWater();

	unsigned int reflectTexture;
	unsigned int bumpTexture;
	unsigned int rawBumpTexture[4];

	unsigned int waterFP;
};

#endif // !defined(AFX_DRAWWATER_H__B59D3FE0_03FC_4A16_AEE4_6384895BD3AE__INCLUDED_)
