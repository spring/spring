#ifndef GLOBALAICALLBACK_H
#define GLOBALAICALLBACK_H

#include "IGlobalAICallback.h"
class CGlobalAI;
class CAICheats;

class CGlobalAICallback :
	public IGlobalAICallback
{
public:
	CGlobalAICallback(CGlobalAI* ai);
	~CGlobalAICallback(void);

	CGlobalAI* ai;
	CAICheats* cheats;
	bool noMessages;

	IAICheats* GetCheatInterface();

	void SendTextMsg(const char* text,int priority);

	int GetCurrentFrame();
	int GetMyTeam();
	int GetMyAllyTeam();

	void* CreateSharedMemArea(char* name, int size);
	void ReleasedSharedMemArea(char* name);

	int CreateGroup(char* dll);											//creates a group and return the id it was given, return -1 on failure (dll didnt exist etc)
	void EraseGroup(int groupid);											//erases a specified group
	bool AddUnitToGroup(int unitid,int groupid);		//adds a unit to a specific group, if it was previously in a group its removed from that, return false if the group didnt exist or didnt accept the unit
	bool RemoveUnitFromGroup(int unitid);						//removes a unit from its group
	int GetUnitGroup(int unitid);										//returns the group a unit belongs to, -1 if no group
	const std::vector<CommandDescription>* GetGroupCommands(int unitid);	//the commands that this unit can understand, other commands will be ignored
	int GiveGroupOrder(int unitid, Command* c);

	int GiveOrder(int unitid,Command* c);
	void UpdateIcons();

	const vector<CommandDescription>* GetUnitCommands(int unitid);
	const deque<Command>* GetCurrentUnitCommands(int unitid);

	int GetUnitAiHint(int unitid);				//integer telling something about the units main function, not implemented yet
	int GetUnitTeam(int unitid);
	int GetUnitAllyTeam(int unitid);
	float GetUnitHealth(int unitid);			//the units current health
	float GetUnitMaxHealth(int unitid);		//the units max health
	float GetUnitSpeed(int unitid);				//the units max speed
	float GetUnitPower(int unitid);				//sort of the measure of the units overall power
	float GetUnitExperience(int unitid);	//how experienced the unit is (0.0-1.0)
	float GetUnitSupply(int unitid);			//how well supplied the unit is (0.0-1.0)
	float GetUnitMorale(int unitid);			//the units morale (0.0-1.0)
	float GetUnitMaxRange(int unitid);		//the furthest any weapon of the unit can fire
	bool IsUnitActivated (int unitid); 
	bool UnitBeingBuilt(int unitid);			//returns true if the unit is currently being built
	const UnitDef* GetUnitDef(int unitid);	//this returns the units unitdef struct from which you can read all the statistics of the unit, dont try to change any values in it, dont use this if you dont have to risk of changes in it
	float3 GetUnitPos(int unitid);
	bool GetUnitResourceInfo(int unitid, UnitResourceInfo* resourceInfo);


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
	const char *GetMapName ();
	const char* GetModName();

	float GetMaxMetal();
	float GetExtractorRadius();	
	float GetMinWind();
	float GetMaxWind();
	float GetTidalStrength();

	float GetElevation(float x,float z);

	int CreateSplineFigure(float3 pos1,float3 pos2,float3 pos3,float3 pos4,float width,int arrow,int lifetime,int group);
	int CreateLineFigure(float3 pos1,float3 pos2,float width,int arrow,int lifetime,int group);
	void SetFigureColor(int group,float red,float green,float blue,float alpha);
	void DeleteFigureGroup(int group);

	void DrawUnit(const char* name,float3 pos,float rotation,int lifetime,int team,bool transparent,bool drawBorder);

	bool CanBuildAt(const UnitDef* unitDef,float3 pos);
	float3 ClosestBuildSite(const UnitDef* unitdef,float3 pos,float searchRadius,int minDist);	//returns the closest position from a position that the building can be built, minDist is the distance in squares that the building must keep to other buildings (to make it easier to create paths through a base)

	float GetMetal();				//stored metal for team
	float GetMetalIncome();				
	float GetMetalUsage();				
	float GetMetalStorage();				//metal storage for team

	float GetEnergy();				//stored energy for team
	float GetEnergyIncome();			
	float GetEnergyUsage();				
	float GetEnergyStorage();				//energy storage for team

	int GetFeatures (int *features, int max);
	int GetFeatures (int *features, int max, const float3& pos, float radius);
	FeatureDef* GetFeatureDef (int feature);
	float GetFeatureHealth (int feature);
	float GetFeatureReclaimLeft (int feature);
};

#endif
