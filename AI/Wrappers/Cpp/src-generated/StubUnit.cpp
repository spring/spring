/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubUnit.h"

#include "IncludesSources.h"

	springai::StubUnit::~StubUnit() {}
	void springai::StubUnit::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubUnit::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubUnit::SetUnitId(int unitId) {
		this->unitId = unitId;
	}
	int springai::StubUnit::GetUnitId() const {
		return unitId;
	}

	void springai::StubUnit::SetLimit(int limit) {
		this->limit = limit;
	}
	int springai::StubUnit::GetLimit() {
		return limit;
	}

	void springai::StubUnit::SetMax(int max) {
		this->max = max;
	}
	int springai::StubUnit::GetMax() {
		return max;
	}

	void springai::StubUnit::SetDef(springai::UnitDef* def) {
		this->def = def;
	}
	springai::UnitDef* springai::StubUnit::GetDef() {
		return def;
	}

	void springai::StubUnit::SetUnitRulesParams(std::vector<springai::UnitRulesParam*> unitRulesParams) {
		this->unitRulesParams = unitRulesParams;
	}
	std::vector<springai::UnitRulesParam*> springai::StubUnit::GetUnitRulesParams() {
		return unitRulesParams;
	}

	springai::UnitRulesParam* springai::StubUnit::GetUnitRulesParamByName(const char* rulesParamName) {
		return 0;
	}

	springai::UnitRulesParam* springai::StubUnit::GetUnitRulesParamById(int rulesParamId) {
		return 0;
	}

	void springai::StubUnit::SetTeam(int team) {
		this->team = team;
	}
	int springai::StubUnit::GetTeam() {
		return team;
	}

	void springai::StubUnit::SetAllyTeam(int allyTeam) {
		this->allyTeam = allyTeam;
	}
	int springai::StubUnit::GetAllyTeam() {
		return allyTeam;
	}

	void springai::StubUnit::SetAiHint(int aiHint) {
		this->aiHint = aiHint;
	}
	int springai::StubUnit::GetAiHint() {
		return aiHint;
	}

	void springai::StubUnit::SetStockpile(int stockpile) {
		this->stockpile = stockpile;
	}
	int springai::StubUnit::GetStockpile() {
		return stockpile;
	}

	void springai::StubUnit::SetStockpileQueued(int stockpileQueued) {
		this->stockpileQueued = stockpileQueued;
	}
	int springai::StubUnit::GetStockpileQueued() {
		return stockpileQueued;
	}

	void springai::StubUnit::SetCurrentFuel(float currentFuel) {
		this->currentFuel = currentFuel;
	}
	float springai::StubUnit::GetCurrentFuel() {
		return currentFuel;
	}

	void springai::StubUnit::SetMaxSpeed(float maxSpeed) {
		this->maxSpeed = maxSpeed;
	}
	float springai::StubUnit::GetMaxSpeed() {
		return maxSpeed;
	}

	void springai::StubUnit::SetMaxRange(float maxRange) {
		this->maxRange = maxRange;
	}
	float springai::StubUnit::GetMaxRange() {
		return maxRange;
	}

	void springai::StubUnit::SetMaxHealth(float maxHealth) {
		this->maxHealth = maxHealth;
	}
	float springai::StubUnit::GetMaxHealth() {
		return maxHealth;
	}

	void springai::StubUnit::SetExperience(float experience) {
		this->experience = experience;
	}
	float springai::StubUnit::GetExperience() {
		return experience;
	}

	void springai::StubUnit::SetGroup(int group) {
		this->group = group;
	}
	int springai::StubUnit::GetGroup() {
		return group;
	}

	void springai::StubUnit::SetCurrentCommands(std::vector<springai::Command*> currentCommands) {
		this->currentCommands = currentCommands;
	}
	std::vector<springai::Command*> springai::StubUnit::GetCurrentCommands() {
		return currentCommands;
	}

	void springai::StubUnit::SetSupportedCommands(std::vector<springai::CommandDescription*> supportedCommands) {
		this->supportedCommands = supportedCommands;
	}
	std::vector<springai::CommandDescription*> springai::StubUnit::GetSupportedCommands() {
		return supportedCommands;
	}

	void springai::StubUnit::SetHealth(float health) {
		this->health = health;
	}
	float springai::StubUnit::GetHealth() {
		return health;
	}

	void springai::StubUnit::SetSpeed(float speed) {
		this->speed = speed;
	}
	float springai::StubUnit::GetSpeed() {
		return speed;
	}

	void springai::StubUnit::SetPower(float power) {
		this->power = power;
	}
	float springai::StubUnit::GetPower() {
		return power;
	}

	float springai::StubUnit::GetResourceUse(Resource* resource) {
		return 0;
	}

	float springai::StubUnit::GetResourceMake(Resource* resource) {
		return 0;
	}

	void springai::StubUnit::SetPos(springai::AIFloat3 pos) {
		this->pos = pos;
	}
	springai::AIFloat3 springai::StubUnit::GetPos() {
		return pos;
	}

	void springai::StubUnit::SetVel(springai::AIFloat3 vel) {
		this->vel = vel;
	}
	springai::AIFloat3 springai::StubUnit::GetVel() {
		return vel;
	}

	void springai::StubUnit::SetActivated(bool isActivated) {
		this->isActivated = isActivated;
	}
	bool springai::StubUnit::IsActivated() {
		return isActivated;
	}

	void springai::StubUnit::SetBeingBuilt(bool isBeingBuilt) {
		this->isBeingBuilt = isBeingBuilt;
	}
	bool springai::StubUnit::IsBeingBuilt() {
		return isBeingBuilt;
	}

	void springai::StubUnit::SetCloaked(bool isCloaked) {
		this->isCloaked = isCloaked;
	}
	bool springai::StubUnit::IsCloaked() {
		return isCloaked;
	}

	void springai::StubUnit::SetParalyzed(bool isParalyzed) {
		this->isParalyzed = isParalyzed;
	}
	bool springai::StubUnit::IsParalyzed() {
		return isParalyzed;
	}

	void springai::StubUnit::SetNeutral(bool isNeutral) {
		this->isNeutral = isNeutral;
	}
	bool springai::StubUnit::IsNeutral() {
		return isNeutral;
	}

	void springai::StubUnit::SetBuildingFacing(int buildingFacing) {
		this->buildingFacing = buildingFacing;
	}
	int springai::StubUnit::GetBuildingFacing() {
		return buildingFacing;
	}

	void springai::StubUnit::SetLastUserOrderFrame(int lastUserOrderFrame) {
		this->lastUserOrderFrame = lastUserOrderFrame;
	}
	int springai::StubUnit::GetLastUserOrderFrame() {
		return lastUserOrderFrame;
	}

	void springai::StubUnit::Build(UnitDef* toBuildUnitDef, const springai::AIFloat3& buildPos, int facing, short options, int timeOut) {
	}

	void springai::StubUnit::Stop(short options, int timeOut) {
	}

	void springai::StubUnit::Wait(short options, int timeOut) {
	}

	void springai::StubUnit::WaitFor(int time, short options, int timeOut) {
	}

	void springai::StubUnit::WaitForDeathOf(Unit* toDieUnit, short options, int timeOut) {
	}

	void springai::StubUnit::WaitForSquadSize(int numUnits, short options, int timeOut) {
	}

	void springai::StubUnit::WaitForAll(short options, int timeOut) {
	}

	void springai::StubUnit::MoveTo(const springai::AIFloat3& toPos, short options, int timeOut) {
	}

	void springai::StubUnit::PatrolTo(const springai::AIFloat3& toPos, short options, int timeOut) {
	}

	void springai::StubUnit::Fight(const springai::AIFloat3& toPos, short options, int timeOut) {
	}

	void springai::StubUnit::Attack(Unit* toAttackUnit, short options, int timeOut) {
	}

	void springai::StubUnit::AttackArea(const springai::AIFloat3& toAttackPos, float radius, short options, int timeOut) {
	}

	void springai::StubUnit::Guard(Unit* toGuardUnit, short options, int timeOut) {
	}

	void springai::StubUnit::AiSelect(short options, int timeOut) {
	}

	void springai::StubUnit::AddToGroup(Group* toGroup, short options, int timeOut) {
	}

	void springai::StubUnit::RemoveFromGroup(short options, int timeOut) {
	}

	void springai::StubUnit::Repair(Unit* toRepairUnit, short options, int timeOut) {
	}

	void springai::StubUnit::SetFireState(int fireState, short options, int timeOut) {
	}

	void springai::StubUnit::SetMoveState(int moveState, short options, int timeOut) {
	}

	void springai::StubUnit::SetBase(const springai::AIFloat3& basePos, short options, int timeOut) {
	}

	void springai::StubUnit::SelfDestruct(short options, int timeOut) {
	}

	void springai::StubUnit::SetWantedMaxSpeed(float wantedMaxSpeed, short options, int timeOut) {
	}

	void springai::StubUnit::LoadUnits(std::vector<springai::Unit*> toLoadUnitIds_list, short options, int timeOut) {
	}

	void springai::StubUnit::LoadUnitsInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {
	}

	void springai::StubUnit::LoadOnto(Unit* transporterUnit, short options, int timeOut) {
	}

	void springai::StubUnit::Unload(const springai::AIFloat3& toPos, Unit* toUnloadUnit, short options, int timeOut) {
	}

	void springai::StubUnit::UnloadUnitsInArea(const springai::AIFloat3& toPos, float radius, short options, int timeOut) {
	}

	void springai::StubUnit::SetOn(bool on, short options, int timeOut) {
	}

	void springai::StubUnit::ReclaimUnit(Unit* toReclaimUnit, short options, int timeOut) {
	}

	void springai::StubUnit::ReclaimFeature(Feature* toReclaimFeature, short options, int timeOut) {
	}

	void springai::StubUnit::ReclaimInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {
	}

	void springai::StubUnit::Cloak(bool cloak, short options, int timeOut) {
	}

	void springai::StubUnit::Stockpile(short options, int timeOut) {
	}

	void springai::StubUnit::DGun(Unit* toAttackUnit, short options, int timeOut) {
	}

	void springai::StubUnit::DGunPosition(const springai::AIFloat3& pos, short options, int timeOut) {
	}

	void springai::StubUnit::RestoreArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {
	}

	void springai::StubUnit::SetRepeat(bool repeat, short options, int timeOut) {
	}

	void springai::StubUnit::SetTrajectory(int trajectory, short options, int timeOut) {
	}

	void springai::StubUnit::Resurrect(Feature* toResurrectFeature, short options, int timeOut) {
	}

	void springai::StubUnit::ResurrectInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {
	}

	void springai::StubUnit::Capture(Unit* toCaptureUnit, short options, int timeOut) {
	}

	void springai::StubUnit::CaptureInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {
	}

	void springai::StubUnit::SetAutoRepairLevel(int autoRepairLevel, short options, int timeOut) {
	}

	void springai::StubUnit::SetIdleMode(int idleMode, short options, int timeOut) {
	}

	void springai::StubUnit::ExecuteCustomCommand(int cmdId, std::vector<float> params_list, short options, int timeOut) {
	}

