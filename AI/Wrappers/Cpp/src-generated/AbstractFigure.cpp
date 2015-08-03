/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractFigure.h"

#include "IncludesSources.h"

	springai::AbstractFigure::~AbstractFigure() {}
	int springai::AbstractFigure::CompareTo(const Figure& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((Figure*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractFigure::Equals(const Figure& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	return true;
}

int springai::AbstractFigure::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);

	return internal_ret;
}

std::string springai::AbstractFigure::ToString() {

	std::string internal_ret = "Figure";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

