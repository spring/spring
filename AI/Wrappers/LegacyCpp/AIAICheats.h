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

#include "ExternalAI/IAICheats.h"

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


	virtual void SetMyHandicap(float handicap);

	virtual void GiveMeMetal(float amount);
	virtual void GiveMeEnergy(float amount);

	virtual int CreateUnit(const char* name, float3 pos);

	virtual const UnitDef* GetUnitDef(int unitid);
	virtual float3 GetUnitPos(int unitid);
	virtual int GetEnemyUnits(int* unitIds, int unitIds_max);
	virtual int GetEnemyUnits(int* unitIds, const float3& pos, float radius,
			int unitIds_max);
	virtual int GetNeutralUnits(int* unitIds, int unitIds_max);
	virtual int GetNeutralUnits(int* unitIds, const float3& pos, float radius,
			int unitIds_max);

	virtual int GetUnitTeam(int unitid);
	virtual int GetUnitAllyTeam(int unitid);
	virtual float GetUnitHealth(int unitid);
	virtual float GetUnitMaxHealth(int unitid);
	virtual float GetUnitPower(int unitid);
	virtual float GetUnitExperience(int unitid);
	virtual bool IsUnitActivated(int unitid);
	virtual bool UnitBeingBuilt(int unitid);
	virtual bool IsUnitNeutral(int unitid);
	virtual bool GetUnitResourceInfo(int unitid,
			UnitResourceInfo* resourceInfo);
	virtual const CCommandQueue* GetCurrentUnitCommands(int unitid);

	virtual int GetBuildingFacing(int unitid);
	virtual bool IsUnitCloaked(int unitid);
	virtual bool IsUnitParalyzed(int unitid);

	virtual bool OnlyPassiveCheats();
	virtual void EnableCheatEvents(bool enable);

	virtual bool GetProperty(int id, int property, void* dst);
	virtual bool GetValue(int id, void* dst);
	virtual int HandleCommand(int commandId, void* data);

private:
	int teamId;
	const SSkirmishAICallback* sAICallback;
	CAIAICallback* aiCallback;
	void setCheatsEnabled(bool enable);
};

#endif // _AIAICHEATS_H
