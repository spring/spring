/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappTeamRulesParam.h"

#include "IncludesSources.h"

	springai::WrappTeamRulesParam::WrappTeamRulesParam(int skirmishAIId, int teamId, int teamRulesParamId) {

		this->skirmishAIId = skirmishAIId;
		this->teamId = teamId;
		this->teamRulesParamId = teamRulesParamId;
	}

	springai::WrappTeamRulesParam::~WrappTeamRulesParam() {

	}

	int springai::WrappTeamRulesParam::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappTeamRulesParam::GetTeamId() const {

		return teamId;
	}

	int springai::WrappTeamRulesParam::GetTeamRulesParamId() const {

		return teamRulesParamId;
	}

	springai::WrappTeamRulesParam::TeamRulesParam* springai::WrappTeamRulesParam::GetInstance(int skirmishAIId, int teamId, int teamRulesParamId) {

		if (teamRulesParamId < 0) {
			return NULL;
		}

		springai::TeamRulesParam* internal_ret = NULL;
		internal_ret = new springai::WrappTeamRulesParam(skirmishAIId, teamId, teamRulesParamId);
		return internal_ret;
	}


	const char* springai::WrappTeamRulesParam::GetName() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Team_TeamRulesParam_getName(this->GetSkirmishAIId(), this->GetTeamId(), this->GetTeamRulesParamId());
		return internal_ret_int;
	}

	float springai::WrappTeamRulesParam::GetValueFloat() {

		float internal_ret_int;

		internal_ret_int = bridged_Team_TeamRulesParam_getValueFloat(this->GetSkirmishAIId(), this->GetTeamId(), this->GetTeamRulesParamId());
		return internal_ret_int;
	}

	const char* springai::WrappTeamRulesParam::GetValueString() {

		const char* internal_ret_int;

		internal_ret_int = bridged_Team_TeamRulesParam_getValueString(this->GetSkirmishAIId(), this->GetTeamId(), this->GetTeamRulesParamId());
		return internal_ret_int;
	}
