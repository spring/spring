/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappWeaponMount.h"

#include "IncludesSources.h"

	springai::WrappWeaponMount::WrappWeaponMount(int skirmishAIId, int unitDefId, int weaponMountId) {

		this->skirmishAIId = skirmishAIId;
		this->unitDefId = unitDefId;
		this->weaponMountId = weaponMountId;
	}

	springai::WrappWeaponMount::~WrappWeaponMount() {

	}

	int springai::WrappWeaponMount::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappWeaponMount::GetUnitDefId() const {

		return unitDefId;
	}

	int springai::WrappWeaponMount::GetWeaponMountId() const {

		return weaponMountId;
	}

	springai::WrappWeaponMount::WeaponMount* springai::WrappWeaponMount::GetInstance(int skirmishAIId, int unitDefId, int weaponMountId) {

		if (weaponMountId < 0) {
			return NULL;
		}

		springai::WeaponMount* internal_ret = NULL;
		internal_ret = new springai::WrappWeaponMount(skirmishAIId, unitDefId, weaponMountId);
		return internal_ret;
	}


	const char* springai::WrappWeaponMount::GetName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_UnitDef_WeaponMount_getName(this->GetSkirmishAIId(), this->GetUnitDefId(), this->GetWeaponMountId());
		return internal_ret_int;
	}

	springai::WeaponDef* springai::WrappWeaponMount::GetWeaponDef() {

		WeaponDef* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_WeaponMount_getWeaponDef(this->GetSkirmishAIId(), this->GetUnitDefId(), this->GetWeaponMountId());
		internal_ret_int_out = springai::WrappWeaponDef::GetInstance(skirmishAIId, internal_ret_int);

		return internal_ret_int_out;
	}

	int springai::WrappWeaponMount::GetSlavedTo() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_WeaponMount_getSlavedTo(this->GetSkirmishAIId(), this->GetUnitDefId(), this->GetWeaponMountId());
		return internal_ret_int;
	}

	springai::AIFloat3 springai::WrappWeaponMount::GetMainDir() {

		float return_posF3_out[3];

		bridged_UnitDef_WeaponMount_getMainDir(this->GetSkirmishAIId(), this->GetUnitDefId(), this->GetWeaponMountId(), return_posF3_out);
		springai::AIFloat3 internal_ret(return_posF3_out[0], return_posF3_out[1], return_posF3_out[2]);

		return internal_ret;
	}

	float springai::WrappWeaponMount::GetMaxAngleDif() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_WeaponMount_getMaxAngleDif(this->GetSkirmishAIId(), this->GetUnitDefId(), this->GetWeaponMountId());
		return internal_ret_int;
	}

	float springai::WrappWeaponMount::GetFuelUsage() {

		float internal_ret_int;

		internal_ret_int = bridged_UnitDef_WeaponMount_getFuelUsage(this->GetSkirmishAIId(), this->GetUnitDefId(), this->GetWeaponMountId());
		return internal_ret_int;
	}

	int springai::WrappWeaponMount::GetBadTargetCategory() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_WeaponMount_getBadTargetCategory(this->GetSkirmishAIId(), this->GetUnitDefId(), this->GetWeaponMountId());
		return internal_ret_int;
	}

	int springai::WrappWeaponMount::GetOnlyTargetCategory() {

		int internal_ret_int;

		internal_ret_int = bridged_UnitDef_WeaponMount_getOnlyTargetCategory(this->GetSkirmishAIId(), this->GetUnitDefId(), this->GetWeaponMountId());
		return internal_ret_int;
	}
