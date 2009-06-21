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

	bool SendResources(float mAmount, float eAmount, int receivingTeamId);
	int SendUnits(const std::vector<int>& unitIds, int receivingTeamId);

	bool PosInCamera(float3 pos, float radius);

	int GetCurrentFrame();
	int GetMyTeam();
	int GetMyAllyTeam();
	int GetPlayerTeam(int playerId);
	const char* GetTeamSide(int teamId);
	void* CreateSharedMemArea(char* name, int size);
	void ReleasedSharedMemArea(char* name);

	int CreateGroup();
	void EraseGroup(int groupId);
	bool AddUnitToGroup(int unitId, int groupId);
	bool RemoveUnitFromGroup(int unitId);
	int GetUnitGroup(int unitId);
	const std::vector<CommandDescription>* GetGroupCommands(int groupId);
	int GiveGroupOrder(int unitId, Command* c);

	int GiveOrder(int unitId,Command* c);
	const std::vector<CommandDescription>* GetUnitCommands(int unitId);
	const CCommandQueue* GetCurrentUnitCommands(int unitId);

	int GetUnitAiHint(int unitId);
	int GetUnitTeam(int unitId);
	int GetUnitAllyTeam(int unitId);
	float GetUnitHealth(int unitId);
	float GetUnitMaxHealth(int unitId);
	float GetUnitSpeed(int unitId);
	float GetUnitPower(int unitId);
	float GetUnitExperience(int unitId);
	float GetUnitMaxRange(int unitId);
	bool IsUnitActivated (int unitId);
	bool UnitBeingBuilt(int unitId);
	const UnitDef* GetUnitDef(int unitId);
	float3 GetUnitPos(int unitId);
	int GetBuildingFacing(int unitId);
	bool IsUnitCloaked(int unitId);
	bool IsUnitParalyzed(int unitId);
	bool IsUnitNeutral(int unitId);
	bool GetUnitResourceInfo(int unitId, UnitResourceInfo* resourceInfo);

	const UnitDef* GetUnitDef(const char* unitName);
private:
	const UnitDef* GetUnitDefById(int unitDefId);
public:

	int InitPath(float3 start, float3 end, int pathType);
	float3 GetNextWaypoint(int pathId);
	void FreePath(int pathId);

	float GetPathLength(float3 start, float3 end, int pathType);

	int GetEnemyUnits(int* unitIds, int unitIds_max);
	int GetEnemyUnitsInRadarAndLos(int* unitIds, int unitIds_max);
	int GetEnemyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max);
	int GetFriendlyUnits(int* unitIds, int unitIds_max);
	int GetFriendlyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max);
	int GetNeutralUnits(int* unitIds, int unitIds_max);
	int GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max);


	int GetMapWidth();
	int GetMapHeight();
	const float* GetHeightMap();
	float GetMinHeight();
	float GetMaxHeight();
	const float* GetSlopeMap();
	const unsigned short* GetLosMap();
	int GetLosMapResolution();
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
	void LineDrawerDrawLineAndIcon(int commandId, const float3& endPos, const float* color);
	void LineDrawerDrawIconAtLastPos(int commandId);
	void LineDrawerBreak(const float3& endPos, const float* color);
	void LineDrawerRestart();
	void LineDrawerRestartSameColor();

	int CreateSplineFigure(float3 pos1, float3 pos2, float3 pos3, float3 pos4, float width, int arrow, int lifetime, int figureGroupId);
	int CreateLineFigure(float3 pos1, float3 pos2, float width, int arrow, int lifetime, int figureGroupId);
	void SetFigureColor(int figureGroupId,float red, float green, float blue, float alpha);
	void DeleteFigureGroup(int figureGroupId);

	void DrawUnit(const char* unitName, float3 pos, float rotation, int lifetime, int teamId, bool transparent, bool drawBorder, int facing);

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

	int GetFeatures(int* featureIds, int max);
	int GetFeatures(int* featureIds, int max, const float3& pos, float radius);
	const FeatureDef* GetFeatureDef(int featureId);
private:
	const FeatureDef* GetFeatureDefById(int featureDefId);
public:
	float GetFeatureHealth(int featureId);
	float GetFeatureReclaimLeft(int featureId);
	float3 GetFeaturePos (int featureId);

	bool GetProperty(int unitId, int property, void* dst);
	bool GetValue(int id, void* dst);
	int HandleCommand(int commandId, void* data);

	int GetFileSize(const char* filename);
	bool ReadFile(const char* filename, void* buffer,int bufferLen);
	int GetFileSize(const char* filename, const char* modes);
	bool ReadFile(const char* filename, const char* modes, void* buffer, int bufferLen);

	int GetNumUnitDefs();
	void GetUnitDefList (const UnitDef** list);

	int GetSelectedUnits(int* unitIds, int unitIds_max);
	float3 GetMousePos();
	int GetMapPoints(PointMarker* pm, int pm_sizeMax, bool includeAllies);
	int GetMapLines(LineMarker* lm, int lm_sizeMax, bool includeAllies);

	float GetUnitDefRadius(int unitDefId);
	float GetUnitDefHeight(int unitDefId);

	const WeaponDef* GetWeapon(const char* weaponName);
private:
	const WeaponDef* GetWeaponDefById(int weaponDefId);
public:

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
