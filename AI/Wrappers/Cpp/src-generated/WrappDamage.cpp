/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappDamage.h"

#include "IncludesSources.h"

	springai::WrappDamage::WrappDamage(int skirmishAIId, int weaponDefId) {

		this->skirmishAIId = skirmishAIId;
		this->weaponDefId = weaponDefId;
	}

	springai::WrappDamage::~WrappDamage() {

	}

	int springai::WrappDamage::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappDamage::GetWeaponDefId() const {

		return weaponDefId;
	}

	springai::WrappDamage::Damage* springai::WrappDamage::GetInstance(int skirmishAIId, int weaponDefId) {

		if (weaponDefId < 0) {
			return NULL;
		}

		springai::Damage* internal_ret = NULL;
		internal_ret = new springai::WrappDamage(skirmishAIId, weaponDefId);
		return internal_ret;
	}


	int springai::WrappDamage::GetParalyzeDamageTime() {

		int internal_ret_int;

		internal_ret_int = bridged_WeaponDef_Damage_getParalyzeDamageTime(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappDamage::GetImpulseFactor() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_Damage_getImpulseFactor(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappDamage::GetImpulseBoost() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_Damage_getImpulseBoost(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappDamage::GetCraterMult() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_Damage_getCraterMult(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	float springai::WrappDamage::GetCraterBoost() {

		float internal_ret_int;

		internal_ret_int = bridged_WeaponDef_Damage_getCraterBoost(this->GetSkirmishAIId(), this->GetWeaponDefId());
		return internal_ret_int;
	}

	std::vector<float> springai::WrappDamage::GetTypes() {

		int types_sizeMax;
		int types_raw_size;
		float* types;
		std::vector<float> types_list;
		int types_size;
		int internal_ret_int;

		types_sizeMax = INT_MAX;
		types = NULL;
		types_size = bridged_WeaponDef_Damage_getTypes(this->GetSkirmishAIId(), this->GetWeaponDefId(), types, types_sizeMax);
		types_sizeMax = types_size;
		types_raw_size = types_size;
		types = new float[types_raw_size];

		internal_ret_int = bridged_WeaponDef_Damage_getTypes(this->GetSkirmishAIId(), this->GetWeaponDefId(), types, types_sizeMax);
		types_list.reserve(types_size);
		for (int i=0; i < types_sizeMax; ++i) {
			types_list.push_back(types[i]);
		}
		delete[] types;

		return types_list;
	}
