// Factory.h: interface for the CFactory class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __FACTORY_H__
#define __FACTORY_H__

#include "Building.h"
#include <string>

using namespace std;

class CFactory : public CBuilding  
{
public:
	CR_DECLARE(CFactory);

	CFactory();
	virtual ~CFactory();
	void StopBuild();
	void StartBuild(string type);
	void Update();
	void DependentDied(CObject* o);
	void FinishedBuilding(void);

	float3 CalcBuildPos(int buildPiece=-1); // supply the build piece to speed up
	int GetBuildPiece();

	void UnitInit (UnitDef* def, int team, const float3& position);

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
