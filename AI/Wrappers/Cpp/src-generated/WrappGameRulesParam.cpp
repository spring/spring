/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappGameRulesParam.h"

#include "IncludesSources.h"

	springai::WrappGameRulesParam::WrappGameRulesParam(int skirmishAIId, int gameRulesParamId) {

		this->skirmishAIId = skirmishAIId;
		this->gameRulesParamId = gameRulesParamId;
	}

	springai::WrappGameRulesParam::~WrappGameRulesParam() {

	}

	int springai::WrappGameRulesParam::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappGameRulesParam::GetGameRulesParamId() const {

		return gameRulesParamId;
	}

	springai::WrappGameRulesParam::GameRulesParam* springai::WrappGameRulesParam::GetInstance(int skirmishAIId, int gameRulesParamId) {

		if (gameRulesParamId < 0) {
			return NULL;
		}

		springai::GameRulesParam* internal_ret = NULL;
		internal_ret = new springai::WrappGameRulesParam(skirmishAIId, gameRulesParamId);
		return internal_ret;
	}


	const char* springai::WrappGameRulesParam::GetName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_GameRulesParam_getName(this->GetSkirmishAIId(), this->GetGameRulesParamId());
		return internal_ret_int;
	}

	float springai::WrappGameRulesParam::GetValueFloat() {

		float internal_ret_int;

		internal_ret_int = bridged_GameRulesParam_getValueFloat(this->GetSkirmishAIId(), this->GetGameRulesParamId());
		return internal_ret_int;
	}

	const char* springai::WrappGameRulesParam::GetValueString() {

		const char* internal_ret_int;

		internal_ret_int = bridged_GameRulesParam_getValueString(this->GetSkirmishAIId(), this->GetGameRulesParamId());
		return internal_ret_int;
	}
