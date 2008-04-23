#ifndef AICALLBACK_H
#define AICALLBACK_H

#include "IAICallback.h"

class CGroupHandler;
class CGroup;

class CAICallback: public IAICallback
{
public:
	CAICallback(int Team, CGroupHandler* GH);
	~CAICallback();

	int team;
	bool noMessages;
	CGroupHandler* gh;
	CGroup* group; // only in case it's a group AI

	void verify();

	void SendStartPos(bool ready, float3 pos);
	void SendTextMsg(const char* text, int zone);
	void SetLastMsgPos(float3 pos);
	void AddNotification(float3 pos, float3 color, float alpha);

	bool SendResources(float mAmount, float eAmount, int receivingTeam);
	int SendUnits(const std::vector<int>& unitIDs, int receivingTeam);

	bool PosInCamera(float3 pos, float radius);

	int GetCurrentFrame();
	int GetMyTeam();
	int GetMyAllyTeam();
	int GetPlayerTeam(int player);
	const char* GetTeamSide(int team);
	void* CreateSharedMemArea(char* name, int size);
	void ReleasedSharedMemArea(char* name);

	int CreateGroup(char* dll, unsigned aiNumber);
	void EraseGroup(int groupid);
	bool AddUnitToGroup(int unitid, int groupid);
	bool RemoveUnitFromGroup(int unitid);
	int GetUnitGroup(int unitid);
	const std::vector<CommandDescription>* GetGroupCommands(int unitid);
	int GiveGroupOrder(int unitid, Command* c);

	int GiveOrder(int unitid,Command* c);
	const std::vector<CommandDescription>* GetUnitCommands(int unitid);
	const CCommandQueue* GetCurrentUnitCommands(int unitid);

	int GetUnitAiHint(int unitid);
	int GetUnitTeam(int unitid);
	int GetUnitAllyTeam(int unitid);
	float GetUnitHealth(int unitid);
	float GetUnitMaxHealth(int unitid);
	float GetUnitSpeed(int unitid);
	float GetUnitPower(int unitid);
	float GetUnitExperience(int unitid);
	float GetUnitMaxRange(int unitid);
	bool IsUnitActivated (int unitid);
	bool UnitBeingBuilt(int unitid);
	const UnitDef* GetUnitDef(int unitid);
	float3 GetUnitPos(int unitid);
	int GetBuildingFacing(int unitid);
	bool IsUnitCloaked(int unitid);
	bool IsUnitParalyzed(int unitid);
	bool IsUnitNeutral(int unitid);
	bool GetUnitResourceInfo(int unitid, UnitResourceInfo* resourceInfo);

	const UnitDef* GetUnitDef(const char* unitName);

	int InitPath(float3 start, float3 end, int pathType);
	float3 GetNextWaypoint(int pathid);
	void FreePath(int pathid);

	float GetPathLength(float3 start, float3 end, int pathType);

	int GetEnemyUnits(int* units);
	int GetEnemyUnitsInRadarAndLos(int* units);
	int GetEnemyUnits(int* units, const float3& pos, float radius);
	int GetFriendlyUnits(int* units);
	int GetFriendlyUnits(int* units, const float3& pos, float radius);
	int GetNeutralUnits(int* units);
	int GetNeutralUnits(int* units, const float3& pos, float radius);


	int GetMapWidth();
	int GetMapHeight();
	const float* GetHeightMap();
	float GetMinHeight();
	float GetMaxHeight();
	const float* GetSlopeMap();
	const unsigned short* GetLosMap();
	const unsigned short* GetRadarMap();
	const unsigned short* GetJammerMap();
	const unsigned char* GetMetalMap();
	const char* GetMapName ();
	const char* GetModName();

	float GetMaxMetal();
	float GetExtractorRadius();
	float GetMinWind();
	float GetMaxWind();
	float GetTidalStrength();
	float GetGravity();

	float GetElevation(float x, float z);

	void LineDrawerStartPath(const float3& pos, const float* color);
	void LineDrawerFinishPath();
	void LineDrawerDrawLine(const float3& endPos, const float* color);
	void LineDrawerDrawLineAndIcon(int cmdID, const float3& endPos, const float* color);
	void LineDrawerDrawIconAtLastPos(int cmdID);
	void LineDrawerBreak(const float3& endPos, const float* color);
	void LineDrawerRestart();
	void LineDrawerRestartSameColor();

	int CreateSplineFigure(float3 pos1, float3 pos2, float3 pos3, float3 pos4, float width, int arrow, int lifetime, int group);
	int CreateLineFigure(float3 pos1, float3 pos2, float width, int arrow, int lifetime, int group);
	void SetFigureColor(int group,float red, float green, float blue, float alpha);
	void DeleteFigureGroup(int group);

	void DrawUnit(const char* name, float3 pos, float rotation, int lifetime, int team, bool transparent, bool drawBorder, int facing);

	bool CanBuildAt(const UnitDef* unitDef, float3 pos, int facing);
	float3 ClosestBuildSite(const UnitDef* unitdef, float3 pos, float searchRadius, int minDist, int facing);

	float GetMetal();
	float GetMetalIncome();
	float GetMetalUsage();
	float GetMetalStorage();

	float GetEnergy();
	float GetEnergyIncome();
	float GetEnergyUsage();
	float GetEnergyStorage();

	int GetFeatures(int* features, int max);
	int GetFeatures(int* features, int max, const float3& pos, float radius);
	const FeatureDef* GetFeatureDef (int feature);
	float GetFeatureHealth(int feature);
	float GetFeatureReclaimLeft(int feature);
	float3 GetFeaturePos (int feature);

	bool GetProperty(int unit, int property, void* dst);
	bool GetValue(int id, void* dst);
	int HandleCommand(int commandId, void* data);

	int GetFileSize(const char* name);
	bool ReadFile(const char* name, void* buffer,int bufferLen);
	int GetFileSize(const char* name, const char* modes);
	bool ReadFile(const char* name, const char* modes, void* buffer, int bufferLen);

	int GetNumUnitDefs();
	void GetUnitDefList (const UnitDef** list);

	int GetSelectedUnits(int* units);
	float3 GetMousePos();
	int GetMapPoints(PointMarker* pm, int maxPoints);
	int GetMapLines(LineMarker* lm, int maxLines);

	float GetUnitDefRadius(int def);
	float GetUnitDefHeight(int def);

	const WeaponDef* GetWeapon(const char* weaponname);

	// false if a unit cannot currently be created
	bool CanBuildUnit(int unitDefID);

	virtual const float3* GetStartPos();

	// NOTES:
	// 1. 'data' can be NULL to skip passing in a string
	// 2. if inSize is less than 0, the data size is calculated using strlen()
	// 3. the return data is subject to lua garbage collection,
	//    copy it if you wish to continue using it
	const char* CallLuaRules(const char* data, int inSize = -1, int* outSize = NULL);
};

#endif /* AICALLBACK_H */
