// Factory.h: interface for the CFactory class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FACTORY_H__446AA0A0_DDDA_4C90_BF21_D437FAA93F68__INCLUDED_)
#define AFX_FACTORY_H__446AA0A0_DDDA_4C90_BF21_D437FAA93F68__INCLUDED_

#pragma warning(disable:4786)

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Building.h"
#include <string>

using namespace std;

class CFactory : public CBuilding  
{
public:
	CFactory(const float3 &pos,int team,UnitDef* unitDef);
	virtual ~CFactory();
	void StopBuild();
	void StartBuild(string type);
	void Update();
	void DependentDied(CObject* o);
	void FinishedBuilding(void);

	float buildSpeed;
	
	bool quedBuild;						//if we have a unit that we want to start to nanolath when script is ready
	string nextBuild;
	CUnit* curBuild;					//unit that we are nanolathing
	bool opening;

	int lastBuild;						//last frame we wanted to build something
	void SendToEmptySpot(CUnit* unit);
	void SlowUpdate(void);
	void ChangeTeam(int newTeam,ChangeType type);
};

#endif // !defined(AFX_FACTORY_H__446AA0A0_DDDA_4C90_BF21_D437FAA93F68__INCLUDED_)
