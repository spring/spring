// GroupAiCallback.h: interface for the CGroupAiCallback class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GROUPAICALLBACK_H__D3CCA6DA_BB40_40E0_A08F_211F4410E6E3__INCLUDED_)
#define AFX_GROUPAICALLBACK_H__D3CCA6DA_BB40_40E0_A08F_211F4410E6E3__INCLUDED_

#pragma warning(disable:4786)

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "IGroupAiCallback.h"
class CGroup;

class CGroupAiCallback : public IGroupAiCallback
{
public:
	CGroupAiCallback(CGroup* group);
	~CGroupAiCallback();

	CGroup* group;

	void SendTextMsg(const char* text,int priority);

	int GetCurrentFrame();
	int GetMyTeam();

	int GiveOrder(int unitid,Command* c);
	void UpdateIcons();
	Command GetOrderPreview();
	virtual bool IsSelected();

	const vector<CommandDescription>* GetUnitCommands(int unitid);
	const deque<Command>* GetCurrentUnitCommands(int unitid);

	int GetUnitLastUserOrder(int unitid);	//last frame the user gave a direct order to a unit, ai should probably leave it be for some time to avoid irritating user
	int GetUnitAihint(int unitid);				//integer telling something about the units main function
	int GetUnitTeam(int unitid);
	float GetUnitHealth(int unitid);			//the units current health
	float GetUnitMaxHealth(int unitid);		//the units max health
	float GetUnitSpeed(int unitid);				//the units max speed
	float GetUnitPower(int unitid);				//sort of the measure of the units overall power
	float GetUnitExperience(int unitid);	//how experienced the unit is (0.0-1.0)
	float GetUnitSupply(int unitid);			//how well supplied the unit is (0.0-1.0)
	float GetUnitMorale(int unitid);			//the units morale (0.0-1.0)
	float GetUnitMaxRange(int unitid);		//the furthest any weapon of the unit can fire
	const UnitDef* GetUnitDef(int unitid);	//this returns the units unitdef struct from which you can read all the statistics of the unit, dont try to change any values in it, dont use this if you dont have to risk of changes in it
	float3 GetUnitPos(int unitid);

	const UnitDef* GetUnitDef(const char* unitName);

	int InitPath(float3 start,float3 end,int pathType);
	float3 GetNextWaypoint(int pathid);
	void FreePath(int pathid);

	float GetPathLength(float3 start,float3 end,int pathType);

	int GetEnemyUnits(int *units);					//returns all known enemy units
	int GetEnemyUnits(int *units,const float3& pos,float radius); //returns all known enemy units within radius from pos
	int GetFriendlyUnits(int *units);					//returns all friendly units
	int GetFriendlyUnits(int *units,const float3& pos,float radius); //returns all friendly units within radius from pos

	int GetMapWidth();
	int GetMapHeight();
	const float* GetHeightMap();
	const unsigned short* GetLosMap();
	const unsigned short* GetRadarMap();		//a square with value zero means you dont have radar to the square, this is 1/8 the resolution of the standard map
	const unsigned short* GetJammerMap();		//a square with value zero means you dont have radar jamming on the square, this is 1/8 the resolution of the standard map
	const unsigned char* GetMetalMap();			//this map shows the metal density on the map, this is half the resolution of the standard map

	float GetElevation(float x,float z);

	int CreateSplineFigure(float3 pos1,float3 pos2,float3 pos3,float3 pos4,float width,int arrow,int lifetime,int group);
	int CreateLineFigure(float3 pos1,float3 pos2,float width,int arrow,int lifetime,int group);
	void SetFigureColor(int group,float red,float green,float blue,float alpha);
	void DeleteFigureGroup(int group);

	void DrawUnit(const char* name,float3 pos,float rotation,int lifetime,int team,bool transparent,bool drawBorder);

	bool CanBuildAt(const UnitDef* unitDef,float3 pos);

	virtual float GetMetal();				//stored metal for team
	virtual float GetEnergy();				//stored energy for team
	virtual float GetMetalStorage();				//metal storage for team
	virtual float GetEnergyStorage();				//energy storage for team
};

#endif // !defined(AFX_GROUPAICALLBACK_H__D3CCA6DA_BB40_40E0_A08F_211F4410E6E3__INCLUDED_)

