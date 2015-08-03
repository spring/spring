/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappUnitRulesParam.h"

#include "IncludesSources.h"

	springai::WrappUnitRulesParam::WrappUnitRulesParam(int skirmishAIId, int unitId, int unitRulesParamId) {

		this->skirmishAIId = skirmishAIId;
		this->unitId = unitId;
		this->unitRulesParamId = unitRulesParamId;
	}

	springai::WrappUnitRulesParam::~WrappUnitRulesParam() {

	}

	int springai::WrappUnitRulesParam::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappUnitRulesParam::GetUnitId() const {

		return unitId;
	}

	int springai::WrappUnitRulesParam::GetUnitRulesParamId() const {

		return unitRulesParamId;
	}

	springai::WrappUnitRulesParam::UnitRulesParam* springai::WrappUnitRulesParam::GetInstance(int skirmishAIId, int unitId, int unitRulesParamId) {

		if (unitRulesParamId < 0) {
			return NULL;
		}

		springai::UnitRulesParam* internal_ret = NULL;
		internal_ret = new springai::WrappUnitRulesParam(skirmishAIId, unitId, unitRulesParamId);
		return internal_ret;
	}


	const char* springai::WrappUnitRulesParam::GetName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Unit_UnitRulesParam_getName(this->GetSkirmishAIId(), this->GetUnitId(), this->GetUnitRulesParamId());
		return internal_ret_int;
	}

	float springai::WrappUnitRulesParam::GetValueFloat() {

		float internal_ret_int;

		internal_ret_int = bridged_Unit_UnitRulesParam_getValueFloat(this->GetSkirmishAIId(), this->GetUnitId(), this->GetUnitRulesParamId());
		return internal_ret_int;
	}

	const char* springai::WrappUnitRulesParam::GetValueString() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Unit_UnitRulesParam_getValueString(this->GetSkirmishAIId(), this->GetUnitId(), this->GetUnitRulesParamId());
		return internal_ret_int;
	}
