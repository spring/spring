/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappGroup.h"

#include "IncludesSources.h"

	springai::WrappGroup::WrappGroup(int skirmishAIId, int groupId) {

		this->skirmishAIId = skirmishAIId;
		this->groupId = groupId;
	}

	springai::WrappGroup::~WrappGroup() {

	}

	int springai::WrappGroup::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappGroup::GetGroupId() const {

		return groupId;
	}

	springai::WrappGroup::Group* springai::WrappGroup::GetInstance(int skirmishAIId, int groupId) {

		if (groupId < 0) {
			return NULL;
		}

		springai::Group* internal_ret = NULL;
		internal_ret = new springai::WrappGroup(skirmishAIId, groupId);
		return internal_ret;
	}


	std::vector<springai::CommandDescription*> springai::WrappGroup::GetSupportedCommands() {

		std::vector<springai::CommandDescription*> RETURN_SIZE_list;
		int RETURN_SIZE_size;
		int internal_ret_int;

		internal_ret_int = bridged_Group_getSupportedCommands(this->GetSkirmishAIId(), this->GetGroupId());
		RETURN_SIZE_size = internal_ret_int;
		RETURN_SIZE_list.reserve(RETURN_SIZE_size);
		for (int i=0; i < RETURN_SIZE_size; ++i) {
			RETURN_SIZE_list.push_back(springai::WrappGroupSupportedCommand::GetInstance(skirmishAIId, groupId, i));
		}

		return RETURN_SIZE_list;
	}

	bool springai::WrappGroup::IsSelected() {

		bool internal_ret_int;

		internal_ret_int = bridged_Group_isSelected(this->GetSkirmishAIId(), this->GetGroupId());
		return internal_ret_int;
	}

	int springai::WrappGroup::Create() {

		int internal_ret_int;

		internal_ret_int = bridged_Group_create(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	void springai::WrappGroup::Erase() {

		int internal_ret_int;

		internal_ret_int = bridged_Group_erase(this->GetSkirmishAIId(), this->GetGroupId());
	if (internal_ret_int != 0) {
		throw CallbackAIException("erase", internal_ret_int);
	}

	}

	void springai::WrappGroup::Build(UnitDef* toBuildUnitDef, const springai::AIFloat3& buildPos, int facing, short options, int timeOut) {

		int internal_ret_int;

		float buildPos_posF3[3];
		buildPos.LoadInto(buildPos_posF3);
		int toBuildUnitDefId = toBuildUnitDef->GetUnitDefId();

		internal_ret_int = bridged_Group_build(this->GetSkirmishAIId(), this->GetGroupId(), toBuildUnitDefId, buildPos_posF3, facing, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("build", internal_ret_int);
	}

	}

	void springai::WrappGroup::Stop(short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_stop(this->GetSkirmishAIId(), this->GetGroupId(), options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("stop", internal_ret_int);
	}

	}

	void springai::WrappGroup::Wait(short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_wait(this->GetSkirmishAIId(), this->GetGroupId(), options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("wait", internal_ret_int);
	}

	}

	void springai::WrappGroup::WaitFor(int time, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_waitFor(this->GetSkirmishAIId(), this->GetGroupId(), time, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("waitFor", internal_ret_int);
	}

	}

	void springai::WrappGroup::WaitForDeathOf(Unit* toDieUnit, short options, int timeOut) {

		int internal_ret_int;

		int toDieUnitId = toDieUnit->GetUnitId();

		internal_ret_int = bridged_Group_waitForDeathOf(this->GetSkirmishAIId(), this->GetGroupId(), toDieUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("waitForDeathOf", internal_ret_int);
	}

	}

	void springai::WrappGroup::WaitForSquadSize(int numUnits, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_waitForSquadSize(this->GetSkirmishAIId(), this->GetGroupId(), numUnits, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("waitForSquadSize", internal_ret_int);
	}

	}

	void springai::WrappGroup::WaitForAll(short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_waitForAll(this->GetSkirmishAIId(), this->GetGroupId(), options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("waitForAll", internal_ret_int);
	}

	}

	void springai::WrappGroup::MoveTo(const springai::AIFloat3& toPos, short options, int timeOut) {

		int internal_ret_int;

		float toPos_posF3[3];
		toPos.LoadInto(toPos_posF3);

		internal_ret_int = bridged_Group_moveTo(this->GetSkirmishAIId(), this->GetGroupId(), toPos_posF3, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("moveTo", internal_ret_int);
	}

	}

	void springai::WrappGroup::PatrolTo(const springai::AIFloat3& toPos, short options, int timeOut) {

		int internal_ret_int;

		float toPos_posF3[3];
		toPos.LoadInto(toPos_posF3);

		internal_ret_int = bridged_Group_patrolTo(this->GetSkirmishAIId(), this->GetGroupId(), toPos_posF3, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("patrolTo", internal_ret_int);
	}

	}

	void springai::WrappGroup::Fight(const springai::AIFloat3& toPos, short options, int timeOut) {

		int internal_ret_int;

		float toPos_posF3[3];
		toPos.LoadInto(toPos_posF3);

		internal_ret_int = bridged_Group_fight(this->GetSkirmishAIId(), this->GetGroupId(), toPos_posF3, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("fight", internal_ret_int);
	}

	}

	void springai::WrappGroup::Attack(Unit* toAttackUnit, short options, int timeOut) {

		int internal_ret_int;

		int toAttackUnitId = toAttackUnit->GetUnitId();

		internal_ret_int = bridged_Group_attack(this->GetSkirmishAIId(), this->GetGroupId(), toAttackUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("attack", internal_ret_int);
	}

	}

	void springai::WrappGroup::AttackArea(const springai::AIFloat3& toAttackPos, float radius, short options, int timeOut) {

		int internal_ret_int;

		float toAttackPos_posF3[3];
		toAttackPos.LoadInto(toAttackPos_posF3);

		internal_ret_int = bridged_Group_attackArea(this->GetSkirmishAIId(), this->GetGroupId(), toAttackPos_posF3, radius, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("attackArea", internal_ret_int);
	}

	}

	void springai::WrappGroup::Guard(Unit* toGuardUnit, short options, int timeOut) {

		int internal_ret_int;

		int toGuardUnitId = toGuardUnit->GetUnitId();

		internal_ret_int = bridged_Group_guard(this->GetSkirmishAIId(), this->GetGroupId(), toGuardUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("guard", internal_ret_int);
	}

	}

	void springai::WrappGroup::AiSelect(short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_aiSelect(this->GetSkirmishAIId(), this->GetGroupId(), options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("aiSelect", internal_ret_int);
	}

	}

	void springai::WrappGroup::AddToGroup(Group* toGroup, short options, int timeOut) {

		int internal_ret_int;

		int toGroupId = toGroup->GetGroupId();

		internal_ret_int = bridged_Group_addToGroup(this->GetSkirmishAIId(), this->GetGroupId(), toGroupId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("addToGroup", internal_ret_int);
	}

	}

	void springai::WrappGroup::RemoveFromGroup(short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_removeFromGroup(this->GetSkirmishAIId(), this->GetGroupId(), options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("removeFromGroup", internal_ret_int);
	}

	}

	void springai::WrappGroup::Repair(Unit* toRepairUnit, short options, int timeOut) {

		int internal_ret_int;

		int toRepairUnitId = toRepairUnit->GetUnitId();

		internal_ret_int = bridged_Group_repair(this->GetSkirmishAIId(), this->GetGroupId(), toRepairUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("repair", internal_ret_int);
	}

	}

	void springai::WrappGroup::SetFireState(int fireState, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_setFireState(this->GetSkirmishAIId(), this->GetGroupId(), fireState, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setFireState", internal_ret_int);
	}

	}

	void springai::WrappGroup::SetMoveState(int moveState, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_setMoveState(this->GetSkirmishAIId(), this->GetGroupId(), moveState, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setMoveState", internal_ret_int);
	}

	}

	void springai::WrappGroup::SetBase(const springai::AIFloat3& basePos, short options, int timeOut) {

		int internal_ret_int;

		float basePos_posF3[3];
		basePos.LoadInto(basePos_posF3);

		internal_ret_int = bridged_Group_setBase(this->GetSkirmishAIId(), this->GetGroupId(), basePos_posF3, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setBase", internal_ret_int);
	}

	}

	void springai::WrappGroup::SelfDestruct(short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_selfDestruct(this->GetSkirmishAIId(), this->GetGroupId(), options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("selfDestruct", internal_ret_int);
	}

	}

	void springai::WrappGroup::SetWantedMaxSpeed(float wantedMaxSpeed, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_setWantedMaxSpeed(this->GetSkirmishAIId(), this->GetGroupId(), wantedMaxSpeed, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setWantedMaxSpeed", internal_ret_int);
	}

	}

	void springai::WrappGroup::LoadUnits(std::vector<springai::Unit*> toLoadUnitIds_list, short options, int timeOut) {

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

		internal_ret_int = bridged_Group_loadUnits(this->GetSkirmishAIId(), this->GetGroupId(), toLoadUnitIds, toLoadUnitIds_size, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("loadUnits", internal_ret_int);
	}
		delete[] toLoadUnitIds;

	}

	void springai::WrappGroup::LoadUnitsInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Group_loadUnitsInArea(this->GetSkirmishAIId(), this->GetGroupId(), pos_posF3, radius, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("loadUnitsInArea", internal_ret_int);
	}

	}

	void springai::WrappGroup::LoadOnto(Unit* transporterUnit, short options, int timeOut) {

		int internal_ret_int;

		int transporterUnitId = transporterUnit->GetUnitId();

		internal_ret_int = bridged_Group_loadOnto(this->GetSkirmishAIId(), this->GetGroupId(), transporterUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("loadOnto", internal_ret_int);
	}

	}

	void springai::WrappGroup::Unload(const springai::AIFloat3& toPos, Unit* toUnloadUnit, short options, int timeOut) {

		int internal_ret_int;

		float toPos_posF3[3];
		toPos.LoadInto(toPos_posF3);
		int toUnloadUnitId = toUnloadUnit->GetUnitId();

		internal_ret_int = bridged_Group_unload(this->GetSkirmishAIId(), this->GetGroupId(), toPos_posF3, toUnloadUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("unload", internal_ret_int);
	}

	}

	void springai::WrappGroup::UnloadUnitsInArea(const springai::AIFloat3& toPos, float radius, short options, int timeOut) {

		int internal_ret_int;

		float toPos_posF3[3];
		toPos.LoadInto(toPos_posF3);

		internal_ret_int = bridged_Group_unloadUnitsInArea(this->GetSkirmishAIId(), this->GetGroupId(), toPos_posF3, radius, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("unloadUnitsInArea", internal_ret_int);
	}

	}

	void springai::WrappGroup::SetOn(bool on, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_setOn(this->GetSkirmishAIId(), this->GetGroupId(), on, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setOn", internal_ret_int);
	}

	}

	void springai::WrappGroup::ReclaimUnit(Unit* toReclaimUnit, short options, int timeOut) {

		int internal_ret_int;

		int toReclaimUnitId = toReclaimUnit->GetUnitId();

		internal_ret_int = bridged_Group_reclaimUnit(this->GetSkirmishAIId(), this->GetGroupId(), toReclaimUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("reclaimUnit", internal_ret_int);
	}

	}

	void springai::WrappGroup::ReclaimFeature(Feature* toReclaimFeature, short options, int timeOut) {

		int internal_ret_int;

		int toReclaimFeatureId = toReclaimFeature->GetFeatureId();

		internal_ret_int = bridged_Group_reclaimFeature(this->GetSkirmishAIId(), this->GetGroupId(), toReclaimFeatureId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("reclaimFeature", internal_ret_int);
	}

	}

	void springai::WrappGroup::ReclaimInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Group_reclaimInArea(this->GetSkirmishAIId(), this->GetGroupId(), pos_posF3, radius, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("reclaimInArea", internal_ret_int);
	}

	}

	void springai::WrappGroup::Cloak(bool cloak, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_cloak(this->GetSkirmishAIId(), this->GetGroupId(), cloak, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("cloak", internal_ret_int);
	}

	}

	void springai::WrappGroup::Stockpile(short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_stockpile(this->GetSkirmishAIId(), this->GetGroupId(), options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("stockpile", internal_ret_int);
	}

	}

	void springai::WrappGroup::DGun(Unit* toAttackUnit, short options, int timeOut) {

		int internal_ret_int;

		int toAttackUnitId = toAttackUnit->GetUnitId();

		internal_ret_int = bridged_Group_dGun(this->GetSkirmishAIId(), this->GetGroupId(), toAttackUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("dGun", internal_ret_int);
	}

	}

	void springai::WrappGroup::DGunPosition(const springai::AIFloat3& pos, short options, int timeOut) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Group_dGunPosition(this->GetSkirmishAIId(), this->GetGroupId(), pos_posF3, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("dGunPosition", internal_ret_int);
	}

	}

	void springai::WrappGroup::RestoreArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Group_restoreArea(this->GetSkirmishAIId(), this->GetGroupId(), pos_posF3, radius, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("restoreArea", internal_ret_int);
	}

	}

	void springai::WrappGroup::SetRepeat(bool repeat, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_setRepeat(this->GetSkirmishAIId(), this->GetGroupId(), repeat, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setRepeat", internal_ret_int);
	}

	}

	void springai::WrappGroup::SetTrajectory(int trajectory, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_setTrajectory(this->GetSkirmishAIId(), this->GetGroupId(), trajectory, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setTrajectory", internal_ret_int);
	}

	}

	void springai::WrappGroup::Resurrect(Feature* toResurrectFeature, short options, int timeOut) {

		int internal_ret_int;

		int toResurrectFeatureId = toResurrectFeature->GetFeatureId();

		internal_ret_int = bridged_Group_resurrect(this->GetSkirmishAIId(), this->GetGroupId(), toResurrectFeatureId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("resurrect", internal_ret_int);
	}

	}

	void springai::WrappGroup::ResurrectInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Group_resurrectInArea(this->GetSkirmishAIId(), this->GetGroupId(), pos_posF3, radius, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("resurrectInArea", internal_ret_int);
	}

	}

	void springai::WrappGroup::Capture(Unit* toCaptureUnit, short options, int timeOut) {

		int internal_ret_int;

		int toCaptureUnitId = toCaptureUnit->GetUnitId();

		internal_ret_int = bridged_Group_capture(this->GetSkirmishAIId(), this->GetGroupId(), toCaptureUnitId, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("capture", internal_ret_int);
	}

	}

	void springai::WrappGroup::CaptureInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);

		internal_ret_int = bridged_Group_captureInArea(this->GetSkirmishAIId(), this->GetGroupId(), pos_posF3, radius, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("captureInArea", internal_ret_int);
	}

	}

	void springai::WrappGroup::SetAutoRepairLevel(int autoRepairLevel, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_setAutoRepairLevel(this->GetSkirmishAIId(), this->GetGroupId(), autoRepairLevel, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setAutoRepairLevel", internal_ret_int);
	}

	}

	void springai::WrappGroup::SetIdleMode(int idleMode, short options, int timeOut) {

		int internal_ret_int;

		internal_ret_int = bridged_Group_setIdleMode(this->GetSkirmishAIId(), this->GetGroupId(), idleMode, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setIdleMode", internal_ret_int);
	}

	}

	void springai::WrappGroup::ExecuteCustomCommand(int cmdId, std::vector<float> params_list, short options, int timeOut) {

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

		internal_ret_int = bridged_Group_executeCustomCommand(this->GetSkirmishAIId(), this->GetGroupId(), cmdId, params, params_size, options, timeOut);
	if (internal_ret_int != 0) {
		throw CallbackAIException("executeCustomCommand", internal_ret_int);
	}
		delete[] params;

	}
