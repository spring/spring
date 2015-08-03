/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubWeaponMount.h"

#include "IncludesSources.h"

	springai::StubWeaponMount::~StubWeaponMount() {}
	void springai::StubWeaponMount::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubWeaponMount::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubWeaponMount::SetUnitDefId(int unitDefId) {
		this->unitDefId = unitDefId;
	}
	int springai::StubWeaponMount::GetUnitDefId() const {
		return unitDefId;
	}

	void springai::StubWeaponMount::SetWeaponMountId(int weaponMountId) {
		this->weaponMountId = weaponMountId;
	}
	int springai::StubWeaponMount::GetWeaponMountId() const {
		return weaponMountId;
	}

	void springai::StubWeaponMount::SetName(const char* name) {
		this->name = name;
	}
	const char* springai::StubWeaponMount::GetName() {
		return name;
	}

	void springai::StubWeaponMount::SetWeaponDef(springai::WeaponDef* weaponDef) {
		this->weaponDef = weaponDef;
	}
	springai::WeaponDef* springai::StubWeaponMount::GetWeaponDef() {
		return weaponDef;
	}

	void springai::StubWeaponMount::SetSlavedTo(int slavedTo) {
		this->slavedTo = slavedTo;
	}
	int springai::StubWeaponMount::GetSlavedTo() {
		return slavedTo;
	}

	void springai::StubWeaponMount::SetMainDir(springai::AIFloat3 mainDir) {
		this->mainDir = mainDir;
	}
	springai::AIFloat3 springai::StubWeaponMount::GetMainDir() {
		return mainDir;
	}

	void springai::StubWeaponMount::SetMaxAngleDif(float maxAngleDif) {
		this->maxAngleDif = maxAngleDif;
	}
	float springai::StubWeaponMount::GetMaxAngleDif() {
		return maxAngleDif;
	}

	void springai::StubWeaponMount::SetFuelUsage(float fuelUsage) {
		this->fuelUsage = fuelUsage;
	}
	float springai::StubWeaponMount::GetFuelUsage() {
		return fuelUsage;
	}

	void springai::StubWeaponMount::SetBadTargetCategory(int badTargetCategory) {
		this->badTargetCategory = badTargetCategory;
	}
	int springai::StubWeaponMount::GetBadTargetCategory() {
		return badTargetCategory;
	}

	void springai::StubWeaponMount::SetOnlyTargetCategory(int onlyTargetCategory) {
		this->onlyTargetCategory = onlyTargetCategory;
	}
	int springai::StubWeaponMount::GetOnlyTargetCategory() {
		return onlyTargetCategory;
	}

