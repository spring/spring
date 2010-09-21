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

#ifndef _AIAICHEATS_H
#define _AIAICHEATS_H

#include "IAICheats.h"

class SSkirmishAICallback;
class CAIAICallback;

/**
 * The AI side wrapper over the C AI interface for IAICheats.
 */
class CAIAICheats : public IAICheats {
public:
	CAIAICheats();
	CAIAICheats(int teamId, const SSkirmishAICallback* sAICallback,
			CAIAICallback* aiCallback);


	void SetMyHandicap(float handicap);

	void GiveMeMetal(float amount);
	void GiveMeEnergy(float amount);

	int CreateUnit(const char* name, float3 pos);

	const UnitDef* GetUnitDef(int unitid);

	float3 GetUnitPos(int unitid);
	float3 GetUnitVel(int unitid);

	int GetEnemyUnits(int* unitIds, int unitIds_max);
	int GetEnemyUnits(int* unitIds, const float3& pos, float radius,
			int unitIds_max);
	int GetNeutralUnits(int* unitIds, int unitIds_max);
	int GetNeutralUnits(int* unitIds, const float3& pos, float radius,
			int unitIds_max);

	int GetFeatures(int *features, int max);
	int GetFeatures(int *features, int max, const float3& pos,
			float radius);

	int GetUnitTeam(int unitid);
	int GetUnitAllyTeam(int unitid);
	float GetUnitHealth(int unitid);
	float GetUnitMaxHealth(int unitid);
	float GetUnitPower(int unitid);
	float GetUnitExperience(int unitid);
	bool IsUnitActivated(int unitid);
	bool UnitBeingBuilt(int unitid);
	bool IsUnitNeutral(int unitid);
	bool GetUnitResourceInfo(int unitid, UnitResourceInfo* resourceInfo);
	const CCommandQueue* GetCurrentUnitCommands(int unitid);

	int GetBuildingFacing(int unitid);
	bool IsUnitCloaked(int unitid);
	bool IsUnitParalyzed(int unitid);

	bool OnlyPassiveCheats();
	void EnableCheatEvents(bool enable);

	bool GetProperty(int id, int property, void* dst);
	bool GetValue(int id, void* dst);
	int HandleCommand(int commandId, void* data);

private:
	int teamId;
	const SSkirmishAICallback* sAICallback;
	CAIAICallback* aiCallback;
	void setCheatsEnabled(bool enable);
};

#endif // _AIAICHEATS_H
