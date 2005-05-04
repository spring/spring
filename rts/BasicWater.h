// DrawWater.h: interface for the CDrawWater class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __BASIC_WATER_H__
#define __BASIC_WATER_H__

#include "archdef.h"

#include "BaseWater.h"

class CBasicWater : public CBaseWater  
{
public:
	void Draw();
	void UpdateWater(CGame* game){};
	CBasicWater();
	virtual ~CBasicWater();

	unsigned int texture;
	unsigned int displist;

};

#endif // __BASIC_WATER_H__
