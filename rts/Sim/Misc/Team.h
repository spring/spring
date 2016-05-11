/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef TEAM_H
#define TEAM_H

#include <string>
#include <vector>
#include <list>

#include "TeamBase.h"
#include "TeamStatistics.h"
#include "Sim/Misc/Resource.h"
#include "System/Color.h"
#include "ExternalAI/SkirmishAIKey.h"
#include "Lua/LuaRulesParams.h"

class CUnit;

class CTeam : public TeamBase
{
	CR_DECLARE_DERIVED(CTeam)
public:
	CTeam();

	void ResetResourceState();
	void SlowUpdate();

	bool HaveResources(const SResourcePack& amount) const;
	void AddResources(SResourcePack res, bool useIncomeMultiplier = true);
	bool UseResources(const SResourcePack& res);

	void AddMetal(float amount, bool useIncomeMultiplier = true);
	void AddEnergy(float amount, bool useIncomeMultiplier = true);
	bool UseEnergy(float amount);
	bool UseMetal(float amount);

	void GiveEverythingTo(const unsigned toTeam);

	void Died(bool normalDeath = true);
	void AddPlayer(int playerNum);
	void KillAIs();

	void SetDefaultStartPos();
	void ClampStartPosInStartBox(float3* pos) const;

	void SetMaxUnits(unsigned int n) { maxUnits = n; }
	unsigned int GetMaxUnits() const { return maxUnits; }
	bool AtUnitLimit() const { return (units.size() >= maxUnits); }

	TeamStatistics& GetCurrentStats() { return statHistory.back(); }
	const TeamStatistics& GetCurrentStats() const { return statHistory.back(); }

	CTeam& operator = (const TeamBase& base) {
		TeamBase::operator = (base);
		return *this;
	}

	std::string GetControllerName() const;

	enum AddType {
		AddBuilt,
		AddCaptured,
		AddGiven
	};

	enum RemoveType {
		RemoveDied,
		RemoveCaptured,
		RemoveGiven
	};

	void AddUnit(CUnit* unit, AddType type);
	void RemoveUnit(CUnit* unit, RemoveType type);

public:
	int teamNum;
	unsigned int maxUnits;

	bool isDead;
	bool gaia;
	bool removeUnits;

	std::vector<CUnit*> units;

	SResourcePack res;
	SResourcePack resStorage;

	SResourcePack resPull,    resPrevPull;
	SResourcePack resIncome,  resPrevIncome;
	SResourcePack resExpense, resPrevExpense;
	SResourcePack resShare;
	SResourcePack resDelayedShare; //< excess that might be shared next SlowUpdate
	SResourcePack resSent,     resPrevSent;
	SResourcePack resReceived, resPrevReceived;
	SResourcePack resPrevExcess;

	int nextHistoryEntry;
	std::list<TeamStatistics> statHistory;

	/// mod controlled parameters
	LuaRulesParams::Params  modParams;

	/// unsynced
	SColor origColor;
	float highlight;
};

#endif /* TEAM_H */
