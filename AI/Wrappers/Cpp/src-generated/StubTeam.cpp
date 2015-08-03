/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubTeam.h"

#include "IncludesSources.h"

	springai::StubTeam::~StubTeam() {}
	void springai::StubTeam::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubTeam::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubTeam::SetTeamId(int teamId) {
		this->teamId = teamId;
	}
	int springai::StubTeam::GetTeamId() const {
		return teamId;
	}

	bool springai::StubTeam::HasAIController() {
		return false;
	}

	void springai::StubTeam::SetTeamRulesParams(std::vector<springai::TeamRulesParam*> teamRulesParams) {
		this->teamRulesParams = teamRulesParams;
	}
	std::vector<springai::TeamRulesParam*> springai::StubTeam::GetTeamRulesParams() {
		return teamRulesParams;
	}

	springai::TeamRulesParam* springai::StubTeam::GetTeamRulesParamByName(const char* rulesParamName) {
		return 0;
	}

	springai::TeamRulesParam* springai::StubTeam::GetTeamRulesParamById(int rulesParamId) {
		return 0;
	}

