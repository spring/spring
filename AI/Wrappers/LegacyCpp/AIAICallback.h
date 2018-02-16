/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _AI_AI_CALLBACK_H
#define _AI_AI_CALLBACK_H

#include <memory>

#include "IAICallback.h"

// engine header copies
#include "UnitDef.h"
#include "FeatureDef.h"
#include "WeaponDef.h"

struct SSkirmishAICallback;


namespace springLegacyAI {

/**
 * The AI side wrapper over the C AI interface for IAICallback.
 */
class CAIAICallback : public IAICallback {
public:
	CAIAICallback();
	CAIAICallback(int skirmishAIId, const SSkirmishAICallback* sAICallback);
	~CAIAICallback();

	void SendTextMsg(const char* text, int zone);
	void SetLastMsgPos(float3 pos);
	void AddNotification(float3 pos, float3 color, float alpha);

	bool SendResources(float mAmount, float eAmount, int receivingTeam);

	int SendUnits(const std::vector<int>& unitIDs, int receivingTeam);

	bool PosInCamera(float3 pos, float radius);

	int GetCurrentFrame();

	int GetMySkirmishAIId();
	int GetMyTeam();
	int GetMyAllyTeam();
	int GetPlayerTeam(int player);
	int GetTeams();
	const char* GetTeamSide(int teamId);
	int GetTeamAllyTeam(int teamId);

	float GetTeamMetalCurrent(int teamId);
	float GetTeamMetalIncome(int teamId);
	float GetTeamMetalUsage(int teamId);
	float GetTeamMetalStorage(int teamId);

	float GetTeamEnergyCurrent(int teamId);
	float GetTeamEnergyIncome(int teamId);
	float GetTeamEnergyUsage(int teamId);
	float GetTeamEnergyStorage(int teamId);

	bool IsAllied(int firstAllyTeamId, int secondAllyTeamId);

	void* CreateSharedMemArea(char* name, int size);

	void ReleasedSharedMemArea(char* name);

	int CreateGroup();

	void EraseGroup(int groupid);
	bool AddUnitToGroup(int unitid, int groupid);

	bool RemoveUnitFromGroup(int unitid);
	int GetUnitGroup(int unitid);
	const std::vector<SCommandDescription>* GetGroupCommands(int unitid);
	int GiveGroupOrder(int unitid, Command* c);

	int GiveOrder(int unitid, Command* c);

	const std::vector<SCommandDescription>* GetUnitCommands(int unitid);
	const CCommandQueue* GetCurrentUnitCommands(int unitid);

	int GetMaxUnits();

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
	float3 GetUnitVel(int unitid);

	int GetBuildingFacing(int unitid);
	bool IsUnitCloaked(int unitid);
	bool IsUnitParalyzed(int unitid);
	bool IsUnitNeutral(int unitid);
	bool GetUnitResourceInfo(int unitid, UnitResourceInfo* resourceInfo);

	const UnitDef* GetUnitDef(const char* unitName);
	const UnitDef* GetUnitDefById(int unitDefId);

	int InitPath(float3 start, float3 end, int pathType, float goalRadius);
	float3 GetNextWaypoint(int pathid);
	void FreePath(int pathid);

	float GetPathLength(float3 start, float3 end, int pathType,
			float goalRadius);

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
	const float* GetCornersHeightMap();
	float GetMinHeight();
	float GetMaxHeight();
	const float* GetSlopeMap();

	const unsigned short* GetLosMap();
	int GetLosMapResolution();
	const unsigned short* GetRadarMap();
	const unsigned short* GetJammerMap();
	const unsigned char* GetMetalMap();
	int GetMapHash();
	const char* GetMapName();
	const char* GetMapHumanName();
	int GetModHash();
	const char* GetModName();
	const char* GetModHumanName();
	const char* GetModShortName();
	const char* GetModVersion();

	float GetElevation(float x, float z);

	float GetMaxMetal() const;
	float GetExtractorRadius() const;
	float GetMinWind() const;
	float GetMaxWind() const;
	float GetCurWind() const;
	float GetTidalStrength() const;
	float GetGravity() const;

	void LineDrawerStartPath(const float3& pos, const float* color);
	void LineDrawerFinishPath();
	void LineDrawerDrawLine(const float3& endPos, const float* color);
	void LineDrawerDrawLineAndIcon(int cmdID, const float3& endPos,
			const float* color);
	void LineDrawerDrawIconAtLastPos(int cmdID);
	void LineDrawerBreak(const float3& endPos, const float* color);
	void LineDrawerRestart();
	void LineDrawerRestartSameColor();

	int CreateSplineFigure(float3 pos1, float3 pos2, float3 pos3,
			float3 pos4, float width, int arrow, int lifetime, int group);
	int CreateLineFigure(float3 pos1, float3 pos2, float width,
			int arrow, int lifetime, int group);
	void SetFigureColor(int group, float red, float green, float blue,
			float alpha);
	void DeleteFigureGroup(int group);

	void DrawUnit(const char* name, float3 pos, float rotation,
			int lifetime, int team, bool transparent, bool drawBorder,
			int facing);


	bool IsDebugDrawerEnabled() const;
	void DebugDrawerAddGraphPoint(int, float, float);
	void DebugDrawerDelGraphPoints(int, int);
	void DebugDrawerSetGraphPos(float, float);
	void DebugDrawerSetGraphSize(float, float);
	void DebugDrawerSetGraphLineColor(int, const float3&);
	void DebugDrawerSetGraphLineLabel(int, const char*);

	int DebugDrawerAddOverlayTexture(const float*, int, int);
	void DebugDrawerUpdateOverlayTexture(int, const float*, int, int, int, int);
	void DebugDrawerDelOverlayTexture(int);
	void DebugDrawerSetOverlayTexturePos(int, float, float);
	void DebugDrawerSetOverlayTextureSize(int, float, float);
	void DebugDrawerSetOverlayTextureLabel(int, const char*);


	bool CanBuildAt(const UnitDef* unitDef, float3 pos, int facing);

	float3 ClosestBuildSite(const UnitDef* unitdef, float3 pos,
			float searchRadius, int minDist, int facing);

	bool GetProperty(int id, int property, void* dst);
	bool GetValue(int id, void* dst);
	int HandleCommand(int commandId, void* data);

	int GetFileSize(const char* name);
	bool ReadFile(const char* name, void* buffer, int bufferLen);

	int GetSelectedUnits(int* unitIds, int unitIds_max);
	float3 GetMousePos();
	int GetMapPoints(PointMarker* pm, int pm_sizeMax, bool includeAllies);
	int GetMapLines(LineMarker* lm, int lm_sizeMax, bool includeAllies);

	float GetMetal();
	float GetMetalIncome();
	float GetMetalUsage();
	float GetMetalStorage();

	float GetEnergy();
	float GetEnergyIncome();
	float GetEnergyUsage();
	float GetEnergyStorage();

	int GetFeatures(int *features, int max);
	int GetFeatures(int *features, int max, const float3& pos,
			float radius);
	const FeatureDef* GetFeatureDef(int feature);
	const FeatureDef* GetFeatureDefById(int featureDefId);
	float GetFeatureHealth(int feature);
	float GetFeatureReclaimLeft(int feature);
	float3 GetFeaturePos(int feature);

	int GetNumUnitDefs();
	void GetUnitDefList(const UnitDef** list);
	float GetUnitDefHeight(int def);
	float GetUnitDefRadius(int def);

	const WeaponDef* GetWeapon(const char* weaponName);
	const WeaponDef* GetWeaponDefById(int weaponDefId);

	const float3* GetStartPos();

	unsigned int GetCategoryFlag(const char* categoryName);
	unsigned int GetCategoriesFlag(const char* categoryNames);
	void GetCategoryName(int categoryFlag, char* name, int name_sizeMax);


	std::string CallLuaRules(const char* inData, int inSize);
	std::string CallLuaUI(const char* inData, int inSize);

	std::map<std::string, std::string> GetMyInfo();
	std::map<std::string, std::string> GetMyOptionValues();

private:
	int skirmishAIId;
	const SSkirmishAICallback* sAICallback;

	void init();

	int Internal_GiveOrder(int unitId, int groupId, Command* c);

	// caches
	std::vector<UnitDef> unitDefs;
	std::vector<FeatureDef> featureDefs;
	std::vector<WeaponDef> weaponDefs;

	std::vector<int> unitDefFrames;
	std::vector<int> featureDefFrames;
	std::vector<int> weaponDefFrames;

	// the following three are needed to prevent memory leaks
	std::vector< std::vector<SCommandDescription> > groupPossibleCommands;
	std::vector< std::vector<SCommandDescription> > unitPossibleCommands;
	std::vector< std::unique_ptr<CCommandQueue> > unitCurrentCommandQueues;

	float3 startPos;

	static size_t numClbInstances;

	static std::vector<float> heightMap;
	static std::vector<float> cornersHeightMap;
	static std::vector<float> slopeMap;
	static std::vector<unsigned short> losMap;
	static std::vector<unsigned short> radarMap;
	static std::vector<unsigned short> jammerMap;
	static std::vector<unsigned char> metalMap;
};

} // namespace springLegacyAI

#endif // _AI_AI_CALLBACK_H
