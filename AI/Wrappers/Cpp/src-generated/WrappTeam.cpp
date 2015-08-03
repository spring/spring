/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "WrappTeam.h"

#include "IncludesSources.h"

	springai::WrappTeam::WrappTeam(int skirmishAIId, int teamId) {

		this->skirmishAIId = skirmishAIId;
		this->teamId = teamId;
	}

	springai::WrappTeam::~WrappTeam() {

	}

	int springai::WrappTeam::GetSkirmishAIId() const {

		return skirmishAIId;
	}

	int springai::WrappTeam::GetTeamId() const {

		return teamId;
	}

	springai::WrappTeam::Team* springai::WrappTeam::GetInstance(int skirmishAIId, int teamId) {

		if (teamId < 0) {
			return NULL;
		}

		springai::Team* internal_ret = NULL;
		internal_ret = new springai::WrappTeam(skirmishAIId, teamId);
		return internal_ret;
	}


	bool springai::WrappTeam::HasAIController() {

		bool internal_ret_int;

		internal_ret_int = bridged_Team_hasAIController(this->GetSkirmishAIId(), this->GetTeamId());
		return internal_ret_int;
	}

	std::vector<springai::TeamRulesParam*> springai::WrappTeam::GetTeamRulesParams() {

		int paramIds_sizeMax;
		int paramIds_raw_size;
		int* paramIds;
		std::vector<springai::TeamRulesParam*> paramIds_list;
		int paramIds_size;
		int internal_ret_int;

		paramIds_sizeMax = INT_MAX;
		paramIds = NULL;
		paramIds_size = bridged_Team_getTeamRulesParams(this->GetSkirmishAIId(), this->GetTeamId(), paramIds, paramIds_sizeMax);
		paramIds_sizeMax = paramIds_size;
		paramIds_raw_size = paramIds_size;
		paramIds = new int[paramIds_raw_size];

		internal_ret_int = bridged_Team_getTeamRulesParams(this->GetSkirmishAIId(), this->GetTeamId(), paramIds, paramIds_sizeMax);
		paramIds_list.reserve(paramIds_size);
		for (int i=0; i < paramIds_sizeMax; ++i) {
			paramIds_list.push_back(springai::WrappTeamRulesParam::GetInstance(skirmishAIId, teamId, paramIds[i]));
		}
		delete[] paramIds;

		return paramIds_list;
	}

	springai::TeamRulesParam* springai::WrappTeam::GetTeamRulesParamByName(const char* rulesParamName) {

		TeamRulesParam* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_Team_getTeamRulesParamByName(this->GetSkirmishAIId(), this->GetTeamId(), rulesParamName);
		internal_ret_int_out = springai::WrappTeamRulesParam::GetInstance(skirmishAIId, teamId, internal_ret_int);

		return internal_ret_int_out;
	}

	springai::TeamRulesParam* springai::WrappTeam::GetTeamRulesParamById(int rulesParamId) {

		TeamRulesParam* internal_ret_int_out;
		int internal_ret_int;

		internal_ret_int = bridged_Team_getTeamRulesParamById(this->GetSkirmishAIId(), this->GetTeamId(), rulesParamId);
		internal_ret_int_out = springai::WrappTeamRulesParam::GetInstance(skirmishAIId, teamId, internal_ret_int);

		return internal_ret_int_out;
	}
