/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractTeamRulesParam.h"

#include "IncludesSources.h"

	springai::AbstractTeamRulesParam::~AbstractTeamRulesParam() {}
	int springai::AbstractTeamRulesParam::CompareTo(const TeamRulesParam& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((TeamRulesParam*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetTeamId() < other.GetTeamId()) return BEFORE;
		if (this->GetTeamId() > other.GetTeamId()) return AFTER;
		if (this->GetTeamRulesParamId() < other.GetTeamRulesParamId()) return BEFORE;
		if (this->GetTeamRulesParamId() > other.GetTeamRulesParamId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractTeamRulesParam::Equals(const TeamRulesParam& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetTeamId() != other.GetTeamId()) return false;
	if (this->GetTeamRulesParamId() != other.GetTeamRulesParamId()) return false;
	return true;
}

int springai::AbstractTeamRulesParam::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetTeamId() * (int) (10E5);
	internal_ret += this->GetTeamRulesParamId() * (int) (10E4);

	return internal_ret;
}

std::string springai::AbstractTeamRulesParam::ToString() {

	std::string internal_ret = "TeamRulesParam";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "teamId=%i, ", this->GetTeamId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "teamRulesParamId=%i, ", this->GetTeamRulesParamId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

