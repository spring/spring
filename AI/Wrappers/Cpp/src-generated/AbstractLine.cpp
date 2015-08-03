/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractLine.h"

#include "IncludesSources.h"

	springai::AbstractLine::~AbstractLine() {}
	int springai::AbstractLine::CompareTo(const Line& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((Line*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetLineId() < other.GetLineId()) return BEFORE;
		if (this->GetLineId() > other.GetLineId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractLine::Equals(const Line& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetLineId() != other.GetLineId()) return false;
	return true;
}

int springai::AbstractLine::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetLineId() * (int) (10E5);

	return internal_ret;
}

std::string springai::AbstractLine::ToString() {

	std::string internal_ret = "Line";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "lineId=%i, ", this->GetLineId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

