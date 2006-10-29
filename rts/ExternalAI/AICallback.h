#ifndef AICALLBACK_H
#define AICALLBACK_H

#include "IAICallback.h"

class CGroupHandler;
class CGroup;

class CAICallback : public IAICallback
{
public:
	CAICallback (int Team, CGroupHandler *GH);
	~CAICallback ();

	int team;
	bool noMessages;
	CGroupHandler *gh;
	CGroup *group; // only in case it's a group AI

	void verify ();

	void SendTextMsg(const char* text,int priority);
	void SetLastMsgPos(float3 pos);
	void AddNotification(float3 pos, float3 color, float alpha);
	bool PosInCamera(float3 pos, float radius);

//get the current game time, there is 30 frames per second at normal speed
	int GetCurrentFrame();
	int GetMyTeam();
	int GetMyAllyTeam();
	int GetPlayerTeam(int player);
	// returns the size of the created area, this is initialized to all 0 if not previously created
	//set something to !0 to tell other ais that the area is already initialized when they try to create it
	//the exact internal format of the memory areas is up to the ais to keep consistent
	void* CreateSharedMemArea(char* name, int size);
	//release your reference to a memory area.
	void ReleasedSharedMemArea(char* name);

	int CreateGroup(char* dll,unsigned aiNumber);							//creates a group and return the id it was given, return -1 on failure (dll didnt exist etc)
	void EraseGroup(int groupid);											//erases a specified group
	bool AddUnitToGroup(int unitid,int groupid);		//adds a unit to a specific group, if it was previously in a group its removed from that, return false if the group didnt exist or didnt accept the unit
	bool RemoveUnitFromGroup(int unitid);						//removes a unit from its group
	int GetUnitGroup(int unitid);										//returns the group a unit belongs to, -1 if no group
	const std::vector<CommandDescription>* GetGroupCommands(int unitid);	//the commands that this unit can understand, other commands will be ignored
	int GiveGroupOrder(int unitid, Command* c);

	int GiveOrder(int unitid,Command* c);
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
	float GetUnitMaxRange(int unitid);		//the furthest any weapon of the unit can fire
	bool IsUnitActivated (int unitid); 
	bool UnitBeingBuilt(int unitid);			//returns true if the unit is currently being built
	const UnitDef* GetUnitDef(int unitid);	//this returns the units unitdef struct from which you can read all the statistics of the unit, dont try to change any values in it, dont use this if you dont have to risk of changes in it
	float3 GetUnitPos(int unitid);
	int GetBuildingFacing(int unitid);		//returns building facing (0-3)
	bool IsUnitCloaked(int unitid);
	bool IsUnitParalyzed(int unitid);
	bool GetUnitResourceInfo(int unitid, UnitResourceInfo* resourceInfo);

	const UnitDef* GetUnitDef(const char* unitName);

	int InitPath(float3 start,float3 end,int pathType);
	float3 GetNextWaypoint(int pathid);
	void FreePath(int pathid);

	float GetPathLength(float3 start,float3 end,int pathType);

	int GetEnemyUnits(int *units);					//returns all known enemy units
	int GetEnemyUnitsInRadarAndLos(int *units);
	int GetEnemyUnits(int *units,const float3& pos,float radius); //returns all known enemy units within radius from pos
	int GetFriendlyUnits(int *units);					//returns all friendly units
	int GetFriendlyUnits(int *units,const float3& pos,float radius); //returns all friendly units within radius from pos

	int GetMapWidth();
	int GetMapHeight();
	const float* GetHeightMap();
	float GetMinHeight();
	float GetMaxHeight();
	const float* GetSlopeMap();
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
	float GetGravity();

	float GetElevation(float x,float z);

	void LineDrawerStartPath(const float3& pos, const float* color);
	void LineDrawerFinishPath();
	void LineDrawerDrawLine(const float3& endPos, const float* color);
	void LineDrawerDrawLineAndIcon(int cmdID, const float3& endPos, const float* color);
	void LineDrawerDrawIconAtLastPos(int cmdID);
	void LineDrawerBreak(const float3& endPos, const float* color);
	void LineDrawerRestart();
	void LineDrawerRestartSameColor();

	int CreateSplineFigure(float3 pos1,float3 pos2,float3 pos3,float3 pos4,float width,int arrow,int lifetime,int group);
	int CreateLineFigure(float3 pos1,float3 pos2,float width,int arrow,int lifetime,int group);
	void SetFigureColor(int group,float red,float green,float blue,float alpha);
	void DeleteFigureGroup(int group);

	void DrawUnit(const char* name,float3 pos,float rotation,int lifetime,int team,bool transparent,bool drawBorder,int facing);

	bool CanBuildAt(const UnitDef* unitDef,float3 pos, int facing);
	float3 ClosestBuildSite(const UnitDef* unitdef,float3 pos,float searchRadius,int minDist, int facing);	//returns the closest position from a position that the building can be built, minDist is the distance in squares that the building must keep to other buildings (to make it easier to create paths through a base)

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
	float3 GetFeaturePos (int feature);

	// future callback extensions
	bool GetProperty(int unit, int property, void *dst);
	bool GetValue(int id, void *dst);
	int HandleCommand(int commandId, void *data); 

	int GetFileSize (const char *name); // return -1 when the file doesn't exist
	bool ReadFile (const char *name, void *buffer,int bufferLen); // returns false when file doesn't exist or buffer is too small
	
	int GetNumUnitDefs();
	void GetUnitDefList (const UnitDef** list);

	int GetSelectedUnits(int *units);
	float3 GetMousePos();
	int GetMapPoints(PointMarker *pm, int maxPoints);
	int GetMapLines(LineMarker *lm, int maxLines);

	float GetUnitDefRadius(int def);
	float GetUnitDefHeight(int def);

	const WeaponDef* GetWeapon(const char* weaponname);
};

#endif /* AICALLBACK_H */
