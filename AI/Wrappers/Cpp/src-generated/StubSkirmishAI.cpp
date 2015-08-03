/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "StubSkirmishAI.h"

#include "IncludesSources.h"

	springai::StubSkirmishAI::~StubSkirmishAI() {}
	void springai::StubSkirmishAI::SetSkirmishAIId(int skirmishAIId) {
		this->skirmishAIId = skirmishAIId;
	}
	int springai::StubSkirmishAI::GetSkirmishAIId() const {
		return skirmishAIId;
	}

	void springai::StubSkirmishAI::SetTeamId(int teamId) {
		this->teamId = teamId;
	}
	int springai::StubSkirmishAI::GetTeamId() {
		return teamId;
	}

	void springai::StubSkirmishAI::SetOptionValues(springai::OptionValues* optionValues) {
		this->optionValues = optionValues;
	}
	springai::OptionValues* springai::StubSkirmishAI::GetOptionValues() {
		return optionValues;
	}

	void springai::StubSkirmishAI::SetInfo(springai::Info* info) {
		this->info = info;
	}
	springai::Info* springai::StubSkirmishAI::GetInfo() {
		return info;
	}

