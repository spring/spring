// Generalized callback interface - shared between global AI and group AI
#ifndef IAICALLBACK_H
#define IAICALLBACK_H
#include <vector>
#include <deque>
#include "aibase.h"
#include "float3.h"
#include "Game/command.h"
struct UnitDef;
struct FeatureDef;
// GetProperty() constants
                            // Data buffer will be filled with this:
#define AIVAL_UNITDEF		1  // const UnitDef*
#define AIVAL_CURRENT_FUEL	2  // float

// GetValue() constants
#define AIVAL_NUMDAMAGETYPES 1 // int
#define AIVAL_EXCEPTION_HANDLING 2 // bool
#define AIVAL_MAP_CHECKSUM 3 // unsinged int
#define AIVAL_DEBUG_MODE 4 // bool
#define AIVAL_GAME_MODE 5 // int
#define AIVAL_GAME_PAUSED 6 // bool
#define AIVAL_GAME_SPEED_FACTOR 7 // float
#define AIVAL_GUI_VIEW_RANGE 8 // float
#define AIVAL_GUI_SCREENX 9 // float
#define AIVAL_GUI_SCREENY 10 // float
#define AIVAL_GUI_CAMERA_DIR 11 // float3
#define AIVAL_GUI_CAMERA_POS 12 // float3
#define AIVAL_SCRIPT_FILENAME_DEPRECATED 13 // std::string, hence unsafe, don't use!
#define AIVAL_SCRIPT_FILENAME_CSTR 14       // data points to char buffer of sufficient size
#define AIVAL_LOCATE_FILE_R 15 // char*
#define AIVAL_LOCATE_FILE_W 16 // char*

struct UnitResourceInfo
{
	float metalUse;
	float energyUse;
	float metalMake;
	float energyMake;
};

struct PointMarker
{
	float3 pos;
	unsigned char* color;
	const char* label; // don't store this pointer anywhere, it may become invalid at any time after GetMapPoints()
};

struct LineMarker {
	float3 pos;
	float3 pos2;
	unsigned char* color;
};

// HandleCommand structs:

#define AIHCQuerySubVersionId 0
#define AIHCAddMapPointId 1
#define AIHCAddMapLineId 2
#define AIHCRemoveMapPointId 3

struct AIHCAddMapPoint ///< result of HandleCommand is 1 - ok supported
{
	float3 pos; ///< on this position, only x and z matter
	char *label; ///< create this text on pos in my team color
};

struct AIHCAddMapLine ///< result of HandleCommand is 1 - ok supported
{
	float3 posfrom; ///< draw line from this pos
	float3 posto; ///< to this pos, again only x and z matter
};

struct AIHCRemoveMapPoint ///< result of HandleCommand is 1 - ok supported
{
	float3 pos; ///< remove map points and lines near this point (100 distance)
};


class SPRING_API IAICallback
{
public:
	virtual void SendTextMsg(const char* text,int priority) = 0;
	virtual void SetLastMsgPos(float3 pos) = 0;
	virtual void AddNotification(float3 pos, float3 color, float alpha) = 0;
	virtual bool PosInCamera(float3 pos, float radius) = 0;	// checks if pos is within view of the current camera, using radius as a margin	

	//get the current game time, there is 30 frames per second at normal speed
	virtual int GetCurrentFrame() = 0;
	virtual int GetMyTeam()=0;
	virtual int GetMyAllyTeam()=0;
	virtual int GetPlayerTeam(int player) = 0;
	// returns the size of the created area, this is initialized to all 0 if not previously created
	//set something to !0 to tell other ais that the area is already initialized when they try to create it
	//the exact internal format of the memory areas is up to the ais to keep consistent
	virtual void* CreateSharedMemArea(char* name, int size)=0;
	//release your reference to a memory area.
	virtual void ReleasedSharedMemArea(char* name)=0;

	virtual int CreateGroup(char* dll,unsigned aiNumber) = 0;				//creates a group and return the id it was given, return -1 on failure (dll didnt exist etc)
	virtual void EraseGroup(int groupid) = 0;											//erases a specified group
	virtual bool AddUnitToGroup(int unitid,int groupid) = 0;		//adds a unit to a specific group, if it was previously in a group its removed from that, return false if the group didnt exist or didnt accept the unit
	virtual bool RemoveUnitFromGroup(int unitid) = 0;						//removes a unit from its group
	virtual int GetUnitGroup(int unitid) = 0;										//returns the group a unit belongs to, -1 if no group
	virtual const std::vector<CommandDescription>* GetGroupCommands(int unitid) = 0;	//the commands that this unit can understand, other commands will be ignored
	virtual int GiveGroupOrder(int unitid, Command* c) = 0;

	virtual int GiveOrder(int unitid,Command* c) = 0;

	virtual const vector<CommandDescription>* GetUnitCommands(int unitid) = 0;
	virtual const deque<Command>* GetCurrentUnitCommands(int unitid) = 0;

	virtual int GetUnitAiHint(int unitid) = 0;				//integer telling something about the units main function, not implemented yet
	virtual int GetUnitTeam(int unitid) = 0;
	virtual int GetUnitAllyTeam(int unitid) = 0;
	virtual float GetUnitHealth(int unitid) = 0;			//the units current health
	virtual float GetUnitMaxHealth(int unitid) = 0;		//the units max health
	virtual float GetUnitSpeed(int unitid) = 0;				//the units max speed
	virtual float GetUnitPower(int unitid) = 0;				//sort of the measure of the units overall power
	virtual float GetUnitExperience(int unitid) = 0;	//how experienced the unit is (0.0f-1.0f)
	virtual float GetUnitMaxRange(int unitid) = 0;		//the furthest any weapon of the unit can fire
	virtual bool IsUnitActivated (int unitid) = 0; 
	virtual bool UnitBeingBuilt(int unitid) = 0;			//returns true if the unit is currently being built
	virtual const UnitDef* GetUnitDef(int unitid) = 0;	//this returns the units unitdef struct from which you can read all the statistics of the unit, dont try to change any values in it, dont use this if you dont have to risk of changes in it
	virtual float3 GetUnitPos(int unitid) = 0;
	virtual int GetBuildingFacing(int unitid) = 0;		//returns building facing (0-3)
	virtual bool IsUnitCloaked(int unitid) = 0;
	virtual bool IsUnitParalyzed(int unitid) = 0;
	virtual bool GetUnitResourceInfo(int unitid, UnitResourceInfo* resourceInfo) = 0;

	virtual const UnitDef* GetUnitDef(const char* unitName) = 0;

	//the following functions allows the dll to use the built in pathfinder
	//call InitPath and you get a pathid back
	//use this to call GetNextWaypoint to get subsequent waypoints, the waypoints are centered on 8*8 squares
	//note that the pathfinder calculates the waypoints as needed so dont retrieve them until they are needed
	//the waypoints x and z coordinate is returned in x and z while y is used for error codes
	//>=0 =worked ok,-2=still thinking call again,-1= end of path reached or invalid path
	virtual int InitPath(float3 start,float3 end,int pathType) = 0;
	virtual float3 GetNextWaypoint(int pathid) = 0;
	virtual void FreePath(int pathid) = 0;

	//This function returns the approximate path cost between two points(note that it needs to calculate the complete path so its somewhat expansive)
	virtual float GetPathLength(float3 start,float3 end,int pathType)=0;

	//the following function return the units into arrays that must be allocated by the dll
	//10000 is currently the max amount of units so that should be a safe size for the array
	//the return value indicates how many units was returned, the rest of the array is unchanged
	virtual int GetEnemyUnits(int *units)=0;					//returns all known enemy units (all in los)
	virtual int GetEnemyUnits(int *units,const float3& pos,float radius)=0; //returns all known enemy units within radius from pos
	virtual int GetEnemyUnitsInRadarAndLos(int *units)=0;       //returns all enemy units in radar and los
	virtual int GetFriendlyUnits(int *units)=0;					//returns all friendly units
	virtual int GetFriendlyUnits(int *units,const float3& pos,float radius)=0; //returns all friendly units within radius from pos

	//the following functions are used to get information about the map
	//dont modify or delete any of the pointers returned
	//the maps are stored from top left and each data position is 8*8 in size
	//to get info about a position x,y look at location
	//(int(y/8))*GetMapWidth()+(int(x/8))
	//some of the maps are stored in a lower resolution than this though
	virtual int GetMapWidth()=0;
	virtual int GetMapHeight()=0;
	virtual const float* GetHeightMap()=0;						//this is the height for the center of the squares, this differs slightly from the drawn map since it uses the height at the corners
	virtual const unsigned short* GetLosMap()=0;			//a square with value zero means you dont have los to the square, this is half the resolution of the standard map
	virtual const unsigned short* GetRadarMap()=0;		//a square with value zero means you dont have radar to the square, this is 1/8 the resolution of the standard map
	virtual const unsigned short* GetJammerMap()=0;		//a square with value zero means you dont have radar jamming on the square, this is 1/8 the resolution of the standard map
	virtual const unsigned char* GetMetalMap()=0;			//this map shows the metal density on the map, this is half the resolution of the standard map
	virtual const char* GetMapName()=0;
	virtual const char* GetModName()=0;

	virtual float GetElevation(float x,float z)=0;		//Gets the elevation of the map at position x,z

	virtual float GetMaxMetal()=0;			//returns what metal value 255 in the metal map is worth
	virtual float GetExtractorRadius()=0;			//returns extraction radius for metal extractors
	virtual float GetMinWind()=0;
	virtual float GetMaxWind()=0;
	virtual float GetTidalStrength()=0;
	virtual float GetGravity()=0;

	// the linedrawer interface.
	// this allows you to draw command-like lines and figures.
	// use these only from within CGroupAI::DrawCommands()
	virtual void LineDrawerStartPath(const float3& pos, const float* color)=0;
	virtual void LineDrawerFinishPath()=0;
	virtual void LineDrawerDrawLine(const float3& endPos, const float* color)=0;
	virtual void LineDrawerDrawLineAndIcon(int cmdID, const float3& endPos, const float* color)=0;
	virtual void LineDrawerDrawIconAtLastPos(int cmdID)=0;
	virtual void LineDrawerBreak(const float3& endPos, const float* color)=0;
	virtual void LineDrawerRestart()=0;
	virtual void LineDrawerRestartSameColor()=0;

	//the following functions allow the ai to draw figures in the world
	//each figure is part of a group
	//when creating figures use 0 as group to get a new one, the return value is the new group
	//the lifetime is how many frames a figure should live before being autoremoved, 0 means no removal
	//arrow!=0 means that the figure will get an arrow at the end
	virtual int CreateSplineFigure(float3 pos1,float3 pos2,float3 pos3,float3 pos4,float width,int arrow,int lifetime,int group)=0;	//This function creates a cubic beizer spline figure
	virtual int CreateLineFigure(float3 pos1,float3 pos2,float width,int arrow,int lifetime,int group)=0;
	virtual void SetFigureColor(int group,float red,float green,float blue,float alpha)=0;
	virtual void DeleteFigureGroup(int group)=0;

	//this function allows you to draw units in the map, of course they dont really exist,they just show up on the local players screen
	//they will be drawn in the standard pose before any scripts are run
	//the rotation is in radians,team affects the color of the unit
	virtual void DrawUnit(const char* name,float3 pos,float rotation,int lifetime,int team,bool transparent,bool drawBorder,int facing=0)=0;

	virtual bool CanBuildAt(const UnitDef* unitDef,float3 pos,int facing=0) = 0;
	virtual float3 ClosestBuildSite(const UnitDef* unitdef,float3 pos,float searchRadius,int minDist,int facing=0) = 0;	//returns the closest position from a position that the building can be built, minDist is the distance in squares that the building must keep to other buildings (to make it easier to create paths through a base)

	// future callback extensions
	virtual bool GetProperty(int id, int property, void *dst)=0;
	virtual bool GetValue(int id, void *dst)=0;
	virtual int HandleCommand(int commandId, void *data)=0;

	virtual int GetFileSize (const char *name)=0;// return -1 when the file doesn't exist
	virtual bool ReadFile (const char *name, void *buffer,int bufferLen)=0;// returns false when file doesn't exist or buffer is too small

	// added by alik
	virtual int GetSelectedUnits(int *units)=0; 
	virtual float3 GetMousePos()=0;
	virtual int GetMapPoints(PointMarker *pm, int maxPoints)=0;
	virtual int GetMapLines(LineMarker *lm, int maxLines)=0;

	virtual float GetMetal() = 0;				//stored metal for team
	virtual float GetMetalIncome() = 0;				
	virtual float GetMetalUsage() = 0;				
	virtual float GetMetalStorage() = 0;				//metal storage for team

	virtual float GetEnergy() = 0;				//stored energy for team
	virtual float GetEnergyIncome() = 0;			
	virtual float GetEnergyUsage() = 0;				
	virtual float GetEnergyStorage() = 0;				//energy storage for team

	virtual int GetFeatures (int *features, int max) = 0;
	virtual int GetFeatures (int *features, int max, const float3& pos, float radius) = 0;
	virtual FeatureDef* GetFeatureDef (int feature) = 0;
	virtual float GetFeatureHealth (int feature) = 0;
	virtual float GetFeatureReclaimLeft (int feature) = 0;
	virtual float3 GetFeaturePos (int feature) = 0;

	virtual int GetNumUnitDefs() = 0;
	virtual void GetUnitDefList (const UnitDef** list) = 0;
	virtual float GetUnitDefHeight(int def) = 0; // forces loading of the unit model
	virtual float GetUnitDefRadius(int def) = 0; // forces loading of the unit model
	DECLARE_PURE_VIRTUAL(~IAICallback())
};

#endif /* IAICALLBACK_H */
