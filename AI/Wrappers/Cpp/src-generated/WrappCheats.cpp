/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappCheats.h"

#include "IncludesSources.h"

	springai::WrappCheats::WrappCheats(int skirmishAIId) {

		this->skirmishAIId = skirmishAIId;
	}

	springai::WrappCheats::~WrappCheats() {

	}

	int springai::WrappCheats::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	springai::WrappCheats::Cheats* springai::WrappCheats::GetInstance(int skirmishAIId) {

		if (skirmishAIId < 0) {
			return NULL;
		}

		springai::Cheats* internal_ret = NULL;
		internal_ret = new springai::WrappCheats(skirmishAIId);
		return internal_ret;
	}


	bool springai::WrappCheats::IsEnabled() {

		bool internal_ret_int;

		internal_ret_int = bridged_Cheats_isEnabled(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	bool springai::WrappCheats::SetEnabled(bool enable) {

		bool internal_ret_int;

		internal_ret_int = bridged_Cheats_setEnabled(this->GetSkirmishAIId(), enable);
		return internal_ret_int;
	}

	bool springai::WrappCheats::SetEventsEnabled(bool enabled) {

		bool internal_ret_int;

		internal_ret_int = bridged_Cheats_setEventsEnabled(this->GetSkirmishAIId(), enabled);
		return internal_ret_int;
	}

	bool springai::WrappCheats::IsOnlyPassive() {

		bool internal_ret_int;

		internal_ret_int = bridged_Cheats_isOnlyPassive(this->GetSkirmishAIId());
		return internal_ret_int;
	}

	void springai::WrappCheats::SetMyIncomeMultiplier(float factor) {

		int internal_ret_int;

		internal_ret_int = bridged_Cheats_setMyIncomeMultiplier(this->GetSkirmishAIId(), factor);
	if (internal_ret_int != 0) {
		throw CallbackAIException("setMyIncomeMultiplier", internal_ret_int);
	}

	}

	void springai::WrappCheats::GiveMeResource(Resource* resource, float amount) {

		int internal_ret_int;

		int resourceId = resource->GetResourceId();

		internal_ret_int = bridged_Cheats_giveMeResource(this->GetSkirmishAIId(), resourceId, amount);
	if (internal_ret_int != 0) {
		throw CallbackAIException("giveMeResource", internal_ret_int);
	}

	}

	int springai::WrappCheats::GiveMeUnit(UnitDef* unitDef, const springai::AIFloat3& pos) {

		int internal_ret_int;

		float pos_posF3[3];
		pos.LoadInto(pos_posF3);
		int unitDefId = unitDef->GetUnitDefId();

		internal_ret_int = bridged_Cheats_giveMeUnit(this->GetSkirmishAIId(), unitDefId, pos_posF3);
		return internal_ret_int;
	}
