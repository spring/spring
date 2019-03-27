/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef AI_CALLBACK_H
#define AI_CALLBACK_H

#include "ExternalAI/AILegacySupport.h"
#include "System/float3.h"

#include <string>
#include <vector>
#include <map>

struct Command;
struct UnitDef;
struct FeatureDef;
struct WeaponDef;
struct SCommandDescription;
class CCommandQueue;
class CGroupHandler;
class CGroup;
class CUnit;

/** Generalized legacy callback interface backend */
class CAICallback
{
private:
	int team = -1;

	bool allowOrders = true;

	CGroupHandler* gh = nullptr;

private:
	// utility methods
	void verify();

	/// Returns the unit if the ID is valid
	CUnit* GetUnit(int unitId) const;
	/// Returns the unit if the ID is valid, and the unit is in this AIs team
	CUnit* GetMyTeamUnit(int unitId) const;
	CUnit* GetInSensorRangeUnit(int unitId, unsigned short losFlags) const;
	/// Returns the unit if the ID is valid, and the unit is in LOS
	CUnit* GetInLosUnit(int unitId) const;
	/// Returns the unit if the ID is valid, and the unit is in LOS or Radar
	CUnit* GetInLosAndRadarUnit(int unitId) const;

public:
	CAICallback() = default;
	CAICallback(int teamId);

	void AllowOrders(bool b) { allowOrders = b; }

	void SendStartPos(bool ready, float3 pos);
	void SendTextMsg(const char* text, int zone);
	void SetLastMsgPos(const float3& pos);
	void AddNotification(const float3& pos, const float3& color, float alpha);

	bool SendResources(float mAmount, float eAmount, int receivingTeamId);
	int SendUnits(const std::vector<int>& unitIds, int receivingTeamId);

	bool PosInCamera(const float3& pos, float radius);

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
	const std::vector<const SCommandDescription*>* GetGroupCommands(int groupId);
	int GiveGroupOrder(int unitId, Command* c);

	int GiveOrder(int unitId,Command* c);
	const std::vector<const SCommandDescription*>* GetUnitCommands(int unitId);
	const CCommandQueue* GetCurrentUnitCommands(int unitId);

	int GetUnitAiHint(int unitId);
	int GetUnitTeam(int unitId);
	int GetUnitAllyTeam(int unitId);
	float GetUnitHealth(int unitId);
	/**
	 * Returns the units max health, which may be higher then that of other
	 * units of the same UnitDef, eg. because of experience.
	 */
	float GetUnitMaxHealth(int unitId);
	/**
	 * Returns the units max speed, which may be higher then that of other
	 * units of the same UnitDef, eg. because of experience.
	 */
	float GetUnitSpeed(int unitId);
	/// Returns a sort of measure for the units overall power
	float GetUnitPower(int unitId);
	/// Returns how experienced the unit is (0.0 - 1.0)
	float GetUnitExperience(int unitId);
	/// Returns the furthest distance any weapon of the unit can fire
	float GetUnitMaxRange(int unitId);
	bool IsUnitActivated (int unitId);
	bool UnitBeingBuilt(int unitId);
	const UnitDef* GetUnitDef(int unitId);
	float3 GetUnitPos(int unitId);
	float3 GetUnitVelocity(int unitId);
	int GetBuildingFacing(int unitId);
	bool IsUnitCloaked(int unitId);
	bool IsUnitParalyzed(int unitId);
	bool IsUnitNeutral(int unitId);
	bool GetUnitResourceInfo(int unitId, UnitResourceInfo* resourceInfo);

	const UnitDef* GetUnitDef(const char* unitName);
private:
	const UnitDef* GetUnitDefById(int unitDefId);
public:

	int InitPath(const float3& start, const float3& end, int pathType, float goalRadius);
	float3 GetNextWaypoint(int pathId);
	void FreePath(int pathId);

	float GetPathLength(float3 start, float3 end, int pathType, float goalRadius);
	bool SetPathNodeCost(unsigned int, unsigned int, float);
	float GetPathNodeCost(unsigned int, unsigned int);

	int GetEnemyUnits(int* unitIds, int unitIds_max = -1);
	int GetEnemyUnitsInRadarAndLos(int* unitIds, int unitIds_max = -1);
	int GetEnemyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max = -1);
	int GetFriendlyUnits(int* unitIds, int unitIds_max = -1);
	int GetFriendlyUnits(int* unitIds, const float3& pos, float radius, int unitIds_max = -1);
	int GetNeutralUnits(int* unitIds, int unitIds_max = -1);
	int GetNeutralUnits(int* unitIds, const float3& pos, float radius, int unitIds_max = -1);


	int GetMapWidth();
	int GetMapHeight();
	const float* GetHeightMap();
	const float* GetCornersHeightMap();
	float GetMinHeight();
	float GetMaxHeight();
	const float* GetSlopeMap();
	const unsigned short* GetLosMap();
	int GetLosMapResolution() { return -1; }          // never called, implemented in CAIAICallback
	const unsigned short* GetRadarMap();
	const unsigned short* GetJammerMap();
	const unsigned char* GetMetalMap();
	int GetMapHash() { return 0; }                    // never called, implemented in SSkirmishAICallbackImpl
	const char* GetMapName() { return NULL; }         // never called, implemented in SSkirmishAICallbackImpl
	const char* GetMapHumanName() { return NULL; }    // never called, implemented in SSkirmishAICallbackImpl
	int GetModHash() { return 0; }                    // never called, implemented in SSkirmishAICallbackImpl
	const char* GetModName() { return NULL; }         // never called, implemented in SSkirmishAICallbackImpl
	const char* GetModHumanName() { return NULL; }    // never called, implemented in SSkirmishAICallbackImpl

	float GetMaxMetal() const;
	float GetExtractorRadius() const;
	float GetMinWind() const;
	float GetMaxWind() const;
	float GetCurWind() const;
	float GetTidalStrength() const;
	float GetGravity() const;

	float GetElevation(float x, float z);

	void LineDrawerStartPath(const float3& pos, const float* color);
	void LineDrawerDrawLine(const float3& endPos, const float* color);
	void LineDrawerDrawLineAndIcon(int commandId, const float3& endPos, const float* color);
	void LineDrawerDrawIconAtLastPos(int commandId);
	void LineDrawerBreak(const float3& endPos, const float* color);
	void LineDrawerRestart();
	void LineDrawerRestartSameColor();

	int CreateSplineFigure(const float3& pos1, const float3& pos2, const float3& pos3, const float3& pos4, float width, int arrow, int lifetime, int figureGroupId);
	int CreateLineFigure(const float3& pos1, const float3& pos2, float width, int arrow, int lifetime, int figureGroupId);
	void SetFigureColor(int figureGroupId,float red, float green, float blue, float alpha);
	void DeleteFigureGroup(int figureGroupId);

	void DrawUnit(const char* unitName, const float3& pos, float rotation, int lifetime, int teamId, bool transparent, bool drawBorder, int facing);


	bool IsDebugDrawerEnabled() const;
	// not implemented as members, but as commands via HandleCommand
	void DebugDrawerAddGraphPoint(int, float, float) {}
	void DebugDrawerDelGraphPoints(int, int) {}
	void DebugDrawerSetGraphPos(float, float) {}
	void DebugDrawerSetGraphSize(float, float) {}
	void DebugDrawerSetGraphLineColor(int, const float3&) {}
	void DebugDrawerSetGraphLineLabel(int, const char*) {}

	// these are also not implemented as members
	int DebugDrawerAddOverlayTexture(const float*, int, int) { return 0; }
	void DebugDrawerUpdateOverlayTexture(int, const float*, int, int, int, int) {}
	void DebugDrawerDelOverlayTexture(int) {}
	void DebugDrawerSetOverlayTexturePos(int, float, float) {}
	void DebugDrawerSetOverlayTextureSize(int, float, float) {}
	void DebugDrawerSetOverlayTextureLabel(int, const char*) {}


	bool CanBuildAt(const UnitDef* unitDef, const float3& pos, int facing);
	float3 ClosestBuildSite(const UnitDef* unitdef, const float3& pos, float searchRadius, int minDist, int facing);

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

	int GetSelectedUnits(int* unitIds, int unitIds_max = -1);
	float3 GetMousePos();
	void GetMapPoints(std::vector<PointMarker>& pm, int pm_sizeMax, bool includeAllies);
	void GetMapLines(std::vector<LineMarker>& lm, int lm_sizeMax, bool includeAllies);

	float GetUnitDefRadius(int unitDefId);
	float GetUnitDefHeight(int unitDefId);

	const WeaponDef* GetWeapon(const char* weaponName);
private:
	const WeaponDef* GetWeaponDefById(int weaponDefId);
public:

	// false if a unit cannot currently be created
	bool CanBuildUnit(int unitDefID);

	const float3* GetStartPos();

	// NOTES:
	// 1. 'data' can be NULL to skip passing in a string
	// 2. if inSize is less than 0, the data size is calculated using strlen()
	// 3. the return data is subject to lua garbage collection,
	//    copy it if you wish to continue using it
	const char* CallLuaRules(const char* inData, int inSize, size_t* outSize);
	const char* CallLuaUI(const char* inData, int inSize, size_t* outSize);

	// never called, implemented in SSkirmishAICallbackImpl
	std::map<std::string, std::string> GetMyInfo() { return std::map<std::string, std::string>(); }
	std::map<std::string, std::string> GetMyOptionValues() { return std::map<std::string, std::string>(); }
};

#endif // AI_CALLBACK_H
