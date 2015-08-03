/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubGroup.h"

#include "IncludesSources.h"

	springai::StubGroup::~StubGroup() {}
	void springai::StubGroup::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubGroup::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubGroup::SetGroupId(int groupId) {
		this->groupId = groupId;
	}
	int springai::StubGroup::GetGroupId() const {
		return groupId;
	}

	void springai::StubGroup::SetSupportedCommands(std::vector<springai::CommandDescription*> supportedCommands) {
		this->supportedCommands = supportedCommands;
	}
	std::vector<springai::CommandDescription*> springai::StubGroup::GetSupportedCommands() {
		return supportedCommands;
	}

	void springai::StubGroup::SetSelected(bool isSelected) {
		this->isSelected = isSelected;
	}
	bool springai::StubGroup::IsSelected() {
		return isSelected;
	}

	int springai::StubGroup::Create() {
		return 0;
	}

	void springai::StubGroup::Erase() {
	}

	void springai::StubGroup::Build(UnitDef* toBuildUnitDef, const springai::AIFloat3& buildPos, int facing, short options, int timeOut) {
	}

	void springai::StubGroup::Stop(short options, int timeOut) {
	}

	void springai::StubGroup::Wait(short options, int timeOut) {
	}

	void springai::StubGroup::WaitFor(int time, short options, int timeOut) {
	}

	void springai::StubGroup::WaitForDeathOf(Unit* toDieUnit, short options, int timeOut) {
	}

	void springai::StubGroup::WaitForSquadSize(int numUnits, short options, int timeOut) {
	}

	void springai::StubGroup::WaitForAll(short options, int timeOut) {
	}

	void springai::StubGroup::MoveTo(const springai::AIFloat3& toPos, short options, int timeOut) {
	}

	void springai::StubGroup::PatrolTo(const springai::AIFloat3& toPos, short options, int timeOut) {
	}

	void springai::StubGroup::Fight(const springai::AIFloat3& toPos, short options, int timeOut) {
	}

	void springai::StubGroup::Attack(Unit* toAttackUnit, short options, int timeOut) {
	}

	void springai::StubGroup::AttackArea(const springai::AIFloat3& toAttackPos, float radius, short options, int timeOut) {
	}

	void springai::StubGroup::Guard(Unit* toGuardUnit, short options, int timeOut) {
	}

	void springai::StubGroup::AiSelect(short options, int timeOut) {
	}

	void springai::StubGroup::AddToGroup(Group* toGroup, short options, int timeOut) {
	}

	void springai::StubGroup::RemoveFromGroup(short options, int timeOut) {
	}

	void springai::StubGroup::Repair(Unit* toRepairUnit, short options, int timeOut) {
	}

	void springai::StubGroup::SetFireState(int fireState, short options, int timeOut) {
	}

	void springai::StubGroup::SetMoveState(int moveState, short options, int timeOut) {
	}

	void springai::StubGroup::SetBase(const springai::AIFloat3& basePos, short options, int timeOut) {
	}

	void springai::StubGroup::SelfDestruct(short options, int timeOut) {
	}

	void springai::StubGroup::SetWantedMaxSpeed(float wantedMaxSpeed, short options, int timeOut) {
	}

	void springai::StubGroup::LoadUnits(std::vector<springai::Unit*> toLoadUnitIds_list, short options, int timeOut) {
	}

	void springai::StubGroup::LoadUnitsInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {
	}

	void springai::StubGroup::LoadOnto(Unit* transporterUnit, short options, int timeOut) {
	}

	void springai::StubGroup::Unload(const springai::AIFloat3& toPos, Unit* toUnloadUnit, short options, int timeOut) {
	}

	void springai::StubGroup::UnloadUnitsInArea(const springai::AIFloat3& toPos, float radius, short options, int timeOut) {
	}

	void springai::StubGroup::SetOn(bool on, short options, int timeOut) {
	}

	void springai::StubGroup::ReclaimUnit(Unit* toReclaimUnit, short options, int timeOut) {
	}

	void springai::StubGroup::ReclaimFeature(Feature* toReclaimFeature, short options, int timeOut) {
	}

	void springai::StubGroup::ReclaimInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {
	}

	void springai::StubGroup::Cloak(bool cloak, short options, int timeOut) {
	}

	void springai::StubGroup::Stockpile(short options, int timeOut) {
	}

	void springai::StubGroup::DGun(Unit* toAttackUnit, short options, int timeOut) {
	}

	void springai::StubGroup::DGunPosition(const springai::AIFloat3& pos, short options, int timeOut) {
	}

	void springai::StubGroup::RestoreArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {
	}

	void springai::StubGroup::SetRepeat(bool repeat, short options, int timeOut) {
	}

	void springai::StubGroup::SetTrajectory(int trajectory, short options, int timeOut) {
	}

	void springai::StubGroup::Resurrect(Feature* toResurrectFeature, short options, int timeOut) {
	}

	void springai::StubGroup::ResurrectInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {
	}

	void springai::StubGroup::Capture(Unit* toCaptureUnit, short options, int timeOut) {
	}

	void springai::StubGroup::CaptureInArea(const springai::AIFloat3& pos, float radius, short options, int timeOut) {
	}

	void springai::StubGroup::SetAutoRepairLevel(int autoRepairLevel, short options, int timeOut) {
	}

	void springai::StubGroup::SetIdleMode(int idleMode, short options, int timeOut) {
	}

	void springai::StubGroup::ExecuteCustomCommand(int cmdId, std::vector<float> params_list, short options, int timeOut) {
	}

