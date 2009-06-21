/*
	Copyright (c) 2008 Robin Vobruba <hoijui.quaero@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _AIAICALLBACK_H
#define _AIAICALLBACK_H

#include "ExternalAI/IAICallback.h"

struct SSkirmishAICallback;

/**
 * The AI side wrapper over the C AI interface for IAICallback.
 */
class CAIAICallback : public IAICallback {
public:
	CAIAICallback();
	CAIAICallback(int teamId, const SSkirmishAICallback* sAICallback);

	virtual void SendTextMsg(const char* text, int zone);
	virtual void SetLastMsgPos(float3 pos);
	virtual void AddNotification(float3 pos, float3 color, float alpha);

	virtual bool SendResources(float mAmount, float eAmount, int receivingTeam);

	virtual int SendUnits(const std::vector<int>& unitIDs, int receivingTeam);

	virtual bool PosInCamera(float3 pos, float radius);

	virtual int GetCurrentFrame();

	virtual int GetMyTeam();
	virtual int GetMyAllyTeam();
	virtual int GetPlayerTeam(int player);
	virtual const char* GetTeamSide(int team);

	virtual void* CreateSharedMemArea(char* name, int size);

	virtual void ReleasedSharedMemArea(char* name);

	virtual int CreateGroup();

	virtual void EraseGroup(int groupid);
	virtual bool AddUnitToGroup(int unitid, int groupid);

	virtual bool RemoveUnitFromGroup(int unitid);
	virtual int GetUnitGroup(int unitid);
	virtual const std::vector<CommandDescription>* GetGroupCommands(int unitid);
	virtual int GiveGroupOrder(int unitid, Command* c);

	virtual int GiveOrder(int unitid, Command* c);

	virtual const std::vector<CommandDescription>* GetUnitCommands(int unitid);
	virtual const CCommandQueue* GetCurrentUnitCommands(int unitid);

	virtual int GetUnitAiHint(int unitid);
	virtual int GetUnitTeam(int unitid);
	virtual int GetUnitAllyTeam(int unitid);
	virtual float GetUnitHealth(int unitid);
	virtual float GetUnitMaxHealth(int unitid);
	virtual float GetUnitSpeed(int unitid);
	virtual float GetUnitPower(int unitid);
	virtual float GetUnitExperience(int unitid);
	virtual float GetUnitMaxRange(int unitid);
	virtual bool IsUnitActivated (int unitid);
	virtual bool UnitBeingBuilt(int unitid);
	virtual const UnitDef* GetUnitDef(int unitid);

	virtual float3 GetUnitPos(int unitid);
	virtual int GetBuildingFacing(int unitid);
	virtual bool IsUnitCloaked(int unitid);
	virtual bool IsUnitParalyzed(int unitid);
	virtual bool IsUnitNeutral(int unitid);
	virtual bool GetUnitResourceInfo(int unitid,
			UnitResourceInfo* resourceInfo);

	virtual const UnitDef* GetUnitDef(const char* unitName);
	virtual const UnitDef* GetUnitDefById(int unitDefId);

	virtual int InitPath(float3 start, float3 end, int pathType);
	virtual float3 GetNextWaypoint(int pathid);
	virtual void FreePath(int pathid);

	virtual float GetPathLength(float3 start, float3 end, int pathType);

	virtual int GetEnemyUnits(int* unitIds, int unitIds_max);
	virtual int GetEnemyUnitsInRadarAndLos(int* unitIds, int unitIds_max);
	virtual int GetEnemyUnits(int* unitIds, const float3& pos, float radius,
			int unitIds_max);
	virtual int GetFriendlyUnits(int* unitIds, int unitIds_max);
	virtual int GetFriendlyUnits(int* unitIds, const float3& pos, float radius,
			int unitIds_max);
	virtual int GetNeutralUnits(int* unitIds, int unitIds_max);
	virtual int GetNeutralUnits(int* unitIds, const float3& pos, float radius,
			int unitIds_max);

	virtual int GetMapWidth();
	virtual int GetMapHeight();
	virtual const float* GetHeightMap();
	virtual float GetMinHeight();
	virtual float GetMaxHeight();
	virtual const float* GetSlopeMap();

	virtual const unsigned short* GetLosMap();
	virtual int GetLosMapResolution();
	virtual const unsigned short* GetRadarMap();
	virtual const unsigned short* GetJammerMap();
	virtual const unsigned char* GetMetalMap();
	virtual const char* GetMapName();
	virtual const char* GetModName();

	virtual float GetElevation(float x, float z);

	virtual float GetMaxMetal();
	virtual float GetExtractorRadius();
	virtual float GetMinWind();
	virtual float GetMaxWind();
	virtual float GetTidalStrength();
	virtual float GetGravity();

	virtual void LineDrawerStartPath(const float3& pos, const float* color);
	virtual void LineDrawerFinishPath();
	virtual void LineDrawerDrawLine(const float3& endPos, const float* color);
	virtual void LineDrawerDrawLineAndIcon(int cmdID, const float3& endPos,
			const float* color);
	virtual void LineDrawerDrawIconAtLastPos(int cmdID);
	virtual void LineDrawerBreak(const float3& endPos, const float* color);
	virtual void LineDrawerRestart();
	virtual void LineDrawerRestartSameColor();

	virtual int CreateSplineFigure(float3 pos1, float3 pos2, float3 pos3,
			float3 pos4, float width, int arrow, int lifetime, int group);
	virtual int CreateLineFigure(float3 pos1, float3 pos2, float width,
			int arrow, int lifetime, int group);
	virtual void SetFigureColor(int group, float red, float green, float blue,
			float alpha);
	virtual void DeleteFigureGroup(int group);

	virtual void DrawUnit(const char* name, float3 pos, float rotation,
			int lifetime, int team, bool transparent, bool drawBorder,
			int facing);

	virtual bool CanBuildAt(const UnitDef* unitDef, float3 pos, int facing);

	virtual float3 ClosestBuildSite(const UnitDef* unitdef, float3 pos,
			float searchRadius, int minDist, int facing);

	virtual bool GetProperty(int id, int property, void* dst);
	virtual bool GetValue(int id, void* dst);
	virtual int HandleCommand(int commandId, void* data);

	virtual int GetFileSize(const char* name);
	virtual bool ReadFile(const char* name, void* buffer, int bufferLen);

	virtual int GetSelectedUnits(int* unitIds, int unitIds_max);
	virtual float3 GetMousePos();
	virtual int GetMapPoints(PointMarker* pm, int pm_sizeMax, bool includeAllies);
	virtual int GetMapLines(LineMarker* lm, int lm_sizeMax, bool includeAllies);

	virtual float GetMetal();
	virtual float GetMetalIncome();
	virtual float GetMetalUsage();
	virtual float GetMetalStorage();

	virtual float GetEnergy();
	virtual float GetEnergyIncome();
	virtual float GetEnergyUsage();
	virtual float GetEnergyStorage();

	virtual int GetFeatures(int *features, int max);
	virtual int GetFeatures(int *features, int max, const float3& pos,
			float radius);
	virtual const FeatureDef* GetFeatureDef(int feature);
	virtual const FeatureDef* GetFeatureDefById(int featureDefId);
	virtual float GetFeatureHealth(int feature);
	virtual float GetFeatureReclaimLeft(int feature);
	virtual float3 GetFeaturePos(int feature);

	virtual int GetNumUnitDefs();
	virtual void GetUnitDefList(const UnitDef** list);
	virtual float GetUnitDefHeight(int def);
	virtual float GetUnitDefRadius(int def);

	virtual const WeaponDef* GetWeapon(const char* weaponName);
	virtual const WeaponDef* GetWeaponDefById(int weaponDefId);

	virtual const float3* GetStartPos();


	virtual const char* CallLuaRules(const char* data, int inSize = -1,
			int* outSize = NULL);

private:
	int teamId;
	const SSkirmishAICallback* sAICallback;
//	int currentFrame;
	void init();
//	void setCurrentFrame(int frame) { currentFrame = frame; }
	int Internal_GiveOrder(int unitId, int groupId, Command* c);

	// caches
	WeaponDef** weaponDefs;
	int* weaponDefFrames;
	UnitDef** unitDefs;
	int* unitDefFrames;
	// the following three are needed to prevent memory leacks
	std::vector<CommandDescription>** groupPossibleCommands;
	std::vector<CommandDescription>** unitPossibleCommands;
	CCommandQueue** unitCurrentCommandQueues;
	FeatureDef** featureDefs;
	int* featureDefFrames;
};

#endif // _AIAICALLBACK_H
