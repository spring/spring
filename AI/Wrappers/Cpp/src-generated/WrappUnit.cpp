/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappUnit.h"

#include "IncludesSources.h"

	springai::WrappUnit::WrappUnit(int skirmishAIId, int unitId) {

		this->skirmishAIId = skirmishAIId;
		this->unitId = unitId;
	}

	springai::WrappUnit::~WrappUnit() {

	}

	int springai::WrappUnit::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappUnit::GetUnitId() const {

		return unitId;
	}

	springai::WrappUnit::Unit* springai::WrappUnit::GetInstance(int skirmishAIId, int unitId) {

		if (unitId <= 0) {
			return NULL;
		}

		springai::Unit* internal_ret = NULL;
		internal_ret = new springai::WrappUnit(skirmishAIId, unitId);
		return internal_ret;
	}


	int springai::WrappUnit::GetLimit() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_getLimit(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	int springai::WrappUnit::GetMax() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_getMax(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	springai::UnitDef* springai::WrappUnit::GetDef() {

		UnitDef* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_Unit_getDef(this->GetSkirmishAIId(), this->GetUnitId());
		internal_ret_int_out = springai::WrappUnitDef::GetInstance(skirmishAIId, internal_ret_int);

		return internal_ret_int_out;
	}

	std::vector<springai::UnitRulesParam*> springai::WrappUnit::GetUnitRulesParams() {

		int paramIds_sizeMax;
		int paramIds_raw_size;
		int* paramIds;
		std::vector<springai::UnitRulesParam*> paramIds_list;
		int paramIds_size;
		int internal_ret_int;

		paramIds_sizeMax = INT_MAX;
		paramIds = NULL;
		paramIds_size = bridged_Unit_getUnitRulesParams(this->GetSkirmishAIId(), this->GetUnitId(), paramIds, paramIds_sizeMax);
		paramIds_sizeMax = paramIds_size;
		paramIds_raw_size = paramIds_size;
		paramIds = new int[paramIds_raw_size];

		internal_ret_int = bridged_Unit_getUnitRulesParams(this->GetSkirmishAIId(), this->GetUnitId(), paramIds, paramIds_sizeMax);
		paramIds_list.reserve(paramIds_size);
		for (int i=0; i < paramIds_sizeMax; ++i) {
			paramIds_list.push_back(springai::WrappUnitRulesParam::GetInstance(skirmishAIId, unitId, paramIds[i]));
		}
		delete[] paramIds;

		return paramIds_list;
	}

	springai::UnitRulesParam* springai::WrappUnit::GetUnitRulesParamByName(const char* rulesParamName) {

		UnitRulesParam* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_Unit_getUnitRulesParamByName(this->GetSkirmishAIId(), this->GetUnitId(), rulesParamName);
		internal_ret_int_out = springai::WrappUnitRulesParam::GetInstance(skirmishAIId, unitId, internal_ret_int);

		return internal_ret_int_out;
	}

	springai::UnitRulesParam* springai::WrappUnit::GetUnitRulesParamById(int rulesParamId) {

		UnitRulesParam* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_Unit_getUnitRulesParamById(this->GetSkirmishAIId(), this->GetUnitId(), rulesParamId);
		internal_ret_int_out = springai::WrappUnitRulesParam::GetInstance(skirmishAIId, unitId, internal_ret_int);

		return internal_ret_int_out;
	}

	int springai::WrappUnit::GetTeam() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_getTeam(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	int springai::WrappUnit::GetAllyTeam() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_getAllyTeam(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	int springai::WrappUnit::GetAiHint() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_getAiHint(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	int springai::WrappUnit::GetStockpile() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_getStockpile(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	int springai::WrappUnit::GetStockpileQueued() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_getStockpileQueued(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	float springai::WrappUnit::GetCurrentFuel() {

		float internal_ret_int;

		internal_ret_int = bridged_Unit_getCurrentFuel(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	float springai::WrappUnit::GetMaxSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_Unit_getMaxSpeed(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	float springai::WrappUnit::GetMaxRange() {

		float internal_ret_int;

		internal_ret_int = bridged_Unit_getMaxRange(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	float springai::WrappUnit::GetMaxHealth() {

		float internal_ret_int;

		internal_ret_int = bridged_Unit_getMaxHealth(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	float springai::WrappUnit::GetExperience() {

		float internal_ret_int;

		internal_ret_int = bridged_Unit_getExperience(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	int springai::WrappUnit::GetGroup() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_getGroup(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	std::vector<springai::Command*> springai::WrappUnit::GetCurrentCommands() {

		std::vector<springai::Command*> RETURN_SIZE_list;
		int RETURN_SIZE_size;
		int internal_ret_int;

		internal_ret_int = bridged_Unit_getCurrentCommands(this->GetSkirmishAIId(), this->GetUnitId());
		RETURN_SIZE_size = internal_ret_int;
		RETURN_SIZE_list.reserve(RETURN_SIZE_size);
		for (int i=0; i < RETURN_SIZE_size; ++i) {
			RETURN_SIZE_list.push_back(springai::WrappCurrentCommand::GetInstance(skirmishAIId, unitId, i));
		}

		return RETURN_SIZE_list;
	}

	std::vector<springai::CommandDescription*> springai::WrappUnit::GetSupportedCommands() {

		std::vector<springai::CommandDescription*> RETURN_SIZE_list;
		int RETURN_SIZE_size;
		int internal_ret_int;

		internal_ret_int = bridged_Unit_getSupportedCommands(this->GetSkirmishAIId(), this->GetUnitId());
		RETURN_SIZE_size = internal_ret_int;
		RETURN_SIZE_list.reserve(RETURN_SIZE_size);
		for (int i=0; i < RETURN_SIZE_size; ++i) {
			RETURN_SIZE_list.push_back(springai::WrappUnitSupportedCommand::GetInstance(skirmishAIId, unitId, i));
		}

		return RETURN_SIZE_list;
	}

	float springai::WrappUnit::GetHealth() {

		float internal_ret_int;

		internal_ret_int = bridged_Unit_getHealth(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	float springai::WrappUnit::GetSpeed() {

		float internal_ret_int;

		internal_ret_int = bridged_Unit_getSpeed(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	float springai::WrappUnit::GetPower() {

		float internal_ret_int;

		internal_ret_int = bridged_Unit_getPower(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	float springai::WrappUnit::GetResourceUse(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Unit_getResourceUse(this->GetSkirmishAIId(), this->GetUnitId(), resourceId);
		return internal_ret_int;
	}

	float springai::WrappUnit::GetResourceMake(Resource* resource) {

		float internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Unit_getResourceMake(this->GetSkirmishAIId(), this->GetUnitId(), resourceId);
		return internal_ret_int;
	}

	springai::AIFloat3 springai::WrappUnit::GetPos() {

		float return_posF3_out[3];

		bridged_Unit_getPos(this->GetSkirmishAIId(), this->GetUnitId(), return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}

	springai::AIFloat3 springai::WrappUnit::GetVel() {

		float return_posF3_out[3];

		bridged_Unit_getVel(this->GetSkirmishAIId(), this->GetUnitId(), return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}

	bool springai::WrappUnit::IsActivated() {

		bool internal_ret_int;

		internal_ret_int = bridged_Unit_isActivated(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	bool springai::WrappUnit::IsBeingBuilt() {

		bool internal_ret_int;

		internal_ret_int = bridged_Unit_isBeingBuilt(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	bool springai::WrappUnit::IsCloaked() {

		bool internal_ret_int;

		internal_ret_int = bridged_Unit_isCloaked(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	bool springai::WrappUnit::IsParalyzed() {

		bool internal_ret_int;

		internal_ret_int = bridged_Unit_isParalyzed(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	bool springai::WrappUnit::IsNeutral() {

		bool internal_ret_int;

		internal_ret_int = bridged_Unit_isNeutral(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	int springai::WrappUnit::GetBuildingFacing() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_getBuildingFacing(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	int springai::WrappUnit::GetLastUserOrderFrame() {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_getLastUserOrderFrame(this->GetSkirmishAIId(), this->GetUnitId());
		return internal_ret_int;
	}

	void springai::WrappUnit::Build(UnitDef* toBuildUnitDef, const springai::AIFloat3& buildPos, int facing, short options, int timeOut) {

		int internal_ret_int;

		float buildPos_posF3[3];
		buildPos.LoadInto(buildPos_posF3);
		int toBuildUnitDefId = toBuildUnitDef->GetUnitDefId();

		internal_ret_int = bridged_Unit_build(this->GetSkirmishAIId(), this->GetUnitId(), toBuildUnitDefId, buildPos_posF3, facing, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("build", internal_ret_int);
	}

	}

	void springai::WrappUnit::Stop(short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_stop(this->GetSkirmishAIId(), this->GetUnitId(), options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("stop", internal_ret_int);
	}

	}

	void springai::WrappUnit::Wait(short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_wait(this->GetSkirmishAIId(), this->GetUnitId(), options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("wait", internal_ret_int);
	}

	}

	void springai::WrappUnit::WaitFor(int time, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_waitFor(this->GetSkirmishAIId(), this->GetUnitId(), time, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("waitFor", internal_ret_int);
	}

	}

	void springai::WrappUnit::WaitForDeathOf(Unit* toDieUnit, short options, int timeOut) {

		int internal_ret_int;

		int toDieUnitId = toDieUnit->GetUnitId();

		internal_ret_int = bridged_Unit_waitForDeathOf(this->GetSkirmishAIId(), this->GetUnitId(), toDieUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("waitForDeathOf", internal_ret_int);
	}

	}

	void springai::WrappUnit::WaitForSquadSize(int numUnits, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_waitForSquadSize(this->GetSkirmishAIId(), this->GetUnitId(), numUnits, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("waitForSquadSize", internal_ret_int);
	}

	}

	void springai::WrappUnit::WaitForAll(short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_waitForAll(this->GetSkirmishAIId(), this->GetUnitId(), options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("waitForAll", internal_ret_int);
	}

	}

	void springai::WrappUnit::MoveTo(const springai::AIFloat3& toPos, short options, int timeOut) {

		int internal_ret_int;

		float toPos_posF3[3];
		toPos.LoadInto(toPos_posF3);

		internal_ret_int = bridged_Unit_moveTo(this->GetSkirmishAIId(), this->GetUnitId(), toPos_posF3, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("moveTo", internal_ret_int);
	}

	}

	void springai::WrappUnit::PatrolTo(const springai::AIFloat3& toPos, short options, int timeOut) {

		int internal_ret_int;

		float toPos_posF3[3];
		toPos.LoadInto(toPos_posF3);

		internal_ret_int = bridged_Unit_patrolTo(this->GetSkirmishAIId(), this->GetUnitId(), toPos_posF3, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("patrolTo", internal_ret_int);
	}

	}

	void springai::WrappUnit::Fight(const springai::AIFloat3& toPos, short options, int timeOut) {

		int internal_ret_int;

		float toPos_posF3[3];
		toPos.LoadInto(toPos_posF3);

		internal_ret_int = bridged_Unit_fight(this->GetSkirmishAIId(), this->GetUnitId(), toPos_posF3, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("fight", internal_ret_int);
	}

	}

	void springai::WrappUnit::Attack(Unit* toAttackUnit, short options, int timeOut) {

		int internal_ret_int;

		int toAttackUnitId = toAttackUnit->GetUnitId();

		internal_ret_int = bridged_Unit_attack(this->GetSkirmishAIId(), this->GetUnitId(), toAttackUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("attack", internal_ret_int);
	}

	}

	void springai::WrappUnit::AttackArea(const springai::AIFloat3& toAttackPos, float radius, short options, int timeOut) {

		int internal_ret_int;

		float toAttackPos_posF3[3];
		toAttackPos.LoadInto(toAttackPos_posF3);

		internal_ret_int = bridged_Unit_attackArea(this->GetSkirmishAIId(), this->GetUnitId(), toAttackPos_posF3, radius, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("attackArea", internal_ret_int);
	}

	}

	void springai::WrappUnit::Guard(Unit* toGuardUnit, short options, int timeOut) {

		int internal_ret_int;

		int toGuardUnitId = toGuardUnit->GetUnitId();

		internal_ret_int = bridged_Unit_guard(this->GetSkirmishAIId(), this->GetUnitId(), toGuardUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("guard", internal_ret_int);
	}

	}

	void springai::WrappUnit::AiSelect(short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_aiSelect(this->GetSkirmishAIId(), this->GetUnitId(), options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("aiSelect", internal_ret_int);
	}

	}

	void springai::WrappUnit::AddToGroup(Group* toGroup, short options, int timeOut) {

		int internal_ret_int;

		int toGroupId = toGroup->GetGroupId();

		internal_ret_int = bridged_Unit_addToGroup(this->GetSkirmishAIId(), this->GetUnitId(), toGroupId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("addToGroup", internal_ret_int);
	}

	}

	void springai::WrappUnit::RemoveFromGroup(short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_removeFromGroup(this->GetSkirmishAIId(), this->GetUnitId(), options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("removeFromGroup", internal_ret_int);
	}

	}

	void springai::WrappUnit::Repair(Unit* toRepairUnit, short options, int timeOut) {

		int internal_ret_int;

		int toRepairUnitId = toRepairUnit->GetUnitId();

		internal_ret_int = bridged_Unit_repair(this->GetSkirmishAIId(), this->GetUnitId(), toRepairUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("repair", internal_ret_int);
	}

	}

	void springai::WrappUnit::SetFireState(int fireState, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_setFireState(this->GetSkirmishAIId(), this->GetUnitId(), fireState, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setFireState", internal_ret_int);
	}

	}

	void springai::WrappUnit::SetMoveState(int moveState, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_setMoveState(this->GetSkirmishAIId(), this->GetUnitId(), moveState, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setMoveState", internal_ret_int);
	}

	}

	void springai::WrappUnit::SetBase(const springai::AIFloat3& basePos, short options, int timeOut) {

		int internal_ret_int;

		float basePos_posF3[3];
		basePos.LoadInto(basePos_posF3);

		internal_ret_int = bridged_Unit_setBase(this->GetSkirmishAIId(), this->GetUnitId(), basePos_posF3, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setBase", internal_ret_int);
	}

	}

	void springai::WrappUnit::SelfDestruct(short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_selfDestruct(this->GetSkirmishAIId(), this->GetUnitId(), options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("selfDestruct", internal_ret_int);
	}

	}

	void springai::WrappUnit::SetWantedMaxSpeed(float wantedMaxSpeed, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_setWantedMaxSpeed(this->GetSkirmishAIId(), this->GetUnitId(), wantedMaxSpeed, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setWantedMaxSpeed", internal_ret_int);
	}

	}

	void springai::WrappUnit::LoadUnits(std::vector<springai::Unit*> toLoadUnitIds_list, short options, int timeOut) {

		int toLoadUnitIds_raw_size;
		int* toLoadUnitIds;
		int toLoadUnitIds_size;
		int internal_ret_int;

		toLoadUnitIds_size = toLoadUnitIds_list.size();
		int _size = toLoadUnitIds_size;
		toLoadUnitIds_raw_size = toLoadUnitIds_size;
		toLoadUnitIds = new int[toLoadUnitIds_raw_size];
		for (int i=0; i < _size; ++i) {
			toLoadUnitIds[i] = toLoadUnitIds_list[i]->GetUnitId();
		}

		internal_ret_int = bridged_Unit_loadUnits(this->GetSkirmishAIId(), this->GetUnitId(), toLoadUnitIds, toLoadUnitIds_size, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("loadUnits", internal_ret_int);
	}
		delete[] toLoadUnitIds;

	}

	void springai::WrappUnit::LoadUnitsInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Unit_loadUnitsInArea(this->GetSkirmishAIId(), this->GetUnitId(), pos_posF3, radius, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("loadUnitsInArea", internal_ret_int);
	}

	}

	void springai::WrappUnit::LoadOnto(Unit* transporterUnit, short options, int timeOut) {

		int internal_ret_int;

		int transporterUnitId = transporterUnit->GetUnitId();

		internal_ret_int = bridged_Unit_loadOnto(this->GetSkirmishAIId(), this->GetUnitId(), transporterUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("loadOnto", internal_ret_int);
	}

	}

	void springai::WrappUnit::Unload(const springai::AIFloat3& toPos, Unit* toUnloadUnit, short options, int timeOut) {

		int internal_ret_int;

		float toPos_posF3[3];
		toPos.LoadInto(toPos_posF3);
		int toUnloadUnitId = toUnloadUnit->GetUnitId();

		internal_ret_int = bridged_Unit_unload(this->GetSkirmishAIId(), this->GetUnitId(), toPos_posF3, toUnloadUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("unload", internal_ret_int);
	}

	}

	void springai::WrappUnit::UnloadUnitsInArea(const springai::AIFloat3& toPos, float radius, short options, int timeOut) {

		int internal_ret_int;

		float toPos_posF3[3];
		toPos.LoadInto(toPos_posF3);

		internal_ret_int = bridged_Unit_unloadUnitsInArea(this->GetSkirmishAIId(), this->GetUnitId(), toPos_posF3, radius, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("unloadUnitsInArea", internal_ret_int);
	}

	}

	void springai::WrappUnit::SetOn(bool on, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_setOn(this->GetSkirmishAIId(), this->GetUnitId(), on, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setOn", internal_ret_int);
	}

	}

	void springai::WrappUnit::ReclaimUnit(Unit* toReclaimUnit, short options, int timeOut) {

		int internal_ret_int;

		int toReclaimUnitId = toReclaimUnit->GetUnitId();

		internal_ret_int = bridged_Unit_reclaimUnit(this->GetSkirmishAIId(), this->GetUnitId(), toReclaimUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("reclaimUnit", internal_ret_int);
	}

	}

	void springai::WrappUnit::ReclaimFeature(Feature* toReclaimFeature, short options, int timeOut) {

		int internal_ret_int;

		int toReclaimFeatureId = toReclaimFeature->GetFeatureId();

		internal_ret_int = bridged_Unit_reclaimFeature(this->GetSkirmishAIId(), this->GetUnitId(), toReclaimFeatureId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("reclaimFeature", internal_ret_int);
	}

	}

	void springai::WrappUnit::ReclaimInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Unit_reclaimInArea(this->GetSkirmishAIId(), this->GetUnitId(), pos_posF3, radius, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("reclaimInArea", internal_ret_int);
	}

	}

	void springai::WrappUnit::Cloak(bool cloak, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_cloak(this->GetSkirmishAIId(), this->GetUnitId(), cloak, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("cloak", internal_ret_int);
	}

	}

	void springai::WrappUnit::Stockpile(short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_stockpile(this->GetSkirmishAIId(), this->GetUnitId(), options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("stockpile", internal_ret_int);
	}

	}

	void springai::WrappUnit::DGun(Unit* toAttackUnit, short options, int timeOut) {

		int internal_ret_int;

		int toAttackUnitId = toAttackUnit->GetUnitId();

		internal_ret_int = bridged_Unit_dGun(this->GetSkirmishAIId(), this->GetUnitId(), toAttackUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("dGun", internal_ret_int);
	}

	}

	void springai::WrappUnit::DGunPosition(const springai::AIFloat3& pos, short options, int timeOut) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Unit_dGunPosition(this->GetSkirmishAIId(), this->GetUnitId(), pos_posF3, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("dGunPosition", internal_ret_int);
	}

	}

	void springai::WrappUnit::RestoreArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Unit_restoreArea(this->GetSkirmishAIId(), this->GetUnitId(), pos_posF3, radius, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("restoreArea", internal_ret_int);
	}

	}

	void springai::WrappUnit::SetRepeat(bool repeat, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_setRepeat(this->GetSkirmishAIId(), this->GetUnitId(), repeat, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setRepeat", internal_ret_int);
	}

	}

	void springai::WrappUnit::SetTrajectory(int trajectory, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_setTrajectory(this->GetSkirmishAIId(), this->GetUnitId(), trajectory, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setTrajectory", internal_ret_int);
	}

	}

	void springai::WrappUnit::Resurrect(Feature* toResurrectFeature, short options, int timeOut) {

		int internal_ret_int;

		int toResurrectFeatureId = toResurrectFeature->GetFeatureId();

		internal_ret_int = bridged_Unit_resurrect(this->GetSkirmishAIId(), this->GetUnitId(), toResurrectFeatureId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("resurrect", internal_ret_int);
	}

	}

	void springai::WrappUnit::ResurrectInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Unit_resurrectInArea(this->GetSkirmishAIId(), this->GetUnitId(), pos_posF3, radius, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("resurrectInArea", internal_ret_int);
	}

	}

	void springai::WrappUnit::Capture(Unit* toCaptureUnit, short options, int timeOut) {

		int internal_ret_int;

		int toCaptureUnitId = toCaptureUnit->GetUnitId();

		internal_ret_int = bridged_Unit_capture(this->GetSkirmishAIId(), this->GetUnitId(), toCaptureUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("capture", internal_ret_int);
	}

	}

	void springai::WrappUnit::CaptureInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Unit_captureInArea(this->GetSkirmishAIId(), this->GetUnitId(), pos_posF3, radius, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("captureInArea", internal_ret_int);
	}

	}

	void springai::WrappUnit::SetAutoRepairLevel(int autoRepairLevel, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_setAutoRepairLevel(this->GetSkirmishAIId(), this->GetUnitId(), autoRepairLevel, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setAutoRepairLevel", internal_ret_int);
	}

	}

	void springai::WrappUnit::SetIdleMode(int idleMode, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Unit_setIdleMode(this->GetSkirmishAIId(), this->GetUnitId(), idleMode, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setIdleMode", internal_ret_int);
	}

	}

	void springai::WrappUnit::ExecuteCustomCommand(int cmdId, std::vector<float> params_list, short options, int timeOut) {

		int params_raw_size;
		float* params;
		int params_size;
		int internal_ret_int;

		params_size = params_list.size();
		int _size = params_size;
		params_raw_size = params_size;
		params = new float[params_raw_size];
		for (int i=0; i < _size; ++i) {
			params[i] = params_list[i];
		}

		internal_ret_int = bridged_Unit_executeCustomCommand(this->GetSkirmishAIId(), this->GetUnitId(), cmdId, params, params_size, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("executeCustomCommand", internal_ret_int);
	}
		delete[] params;

	}
