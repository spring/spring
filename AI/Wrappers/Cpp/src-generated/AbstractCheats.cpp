/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractCheats.h"

#include "IncludesSources.h"

	springai::AbstractCheats::~AbstractCheats() {}
	int springai::AbstractCheats::CompareTo(const Cheats& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((Cheats*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractCheats::Equals(const Cheats& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	return true;
}

int springai::AbstractCheats::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);

	return internal_ret;
}

std::string springai::AbstractCheats::ToString() {

	std::string internal_ret = "Cheats";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

