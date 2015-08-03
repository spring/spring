/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractTeam.h"

#include "IncludesSources.h"

	springai::AbstractTeam::~AbstractTeam() {}
	int springai::AbstractTeam::CompareTo(const Team& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((Team*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetTeamId() < other.GetTeamId()) return BEFORE;
		if (this->GetTeamId() > other.GetTeamId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractTeam::Equals(const Team& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetTeamId() != other.GetTeamId()) return false;
	return true;
}

int springai::AbstractTeam::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetTeamId() * (int) (10E5);

	return internal_ret;
}

std::string springai::AbstractTeam::ToString() {

	std::string internal_ret = "Team";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "teamId=%i, ", this->GetTeamId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

