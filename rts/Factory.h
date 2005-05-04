// Factory.h: interface for the CFactory class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __FACTORY_H__
#define __FACTORY_H__

#include "archdef.h"

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

#endif // __FACTORY_H__
