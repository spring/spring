/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractPoint.h"

#include "IncludesSources.h"

	springai::AbstractPoint::~AbstractPoint() {}
	int springai::AbstractPoint::CompareTo(const Point& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((Point*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetPointId() < other.GetPointId()) return BEFORE;
		if (this->GetPointId() > other.GetPointId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractPoint::Equals(const Point& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetPointId() != other.GetPointId()) return false;
	return true;
}

int springai::AbstractPoint::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetPointId() * (int) (10E5);

	return internal_ret;
}

std::string springai::AbstractPoint::ToString() {

	std::string internal_ret = "Point";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "pointId=%i, ", this->GetPointId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

