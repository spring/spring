/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractDrawer.h"

#include "IncludesSources.h"

	springai::AbstractDrawer::~AbstractDrawer() {}
	int springai::AbstractDrawer::CompareTo(const Drawer& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((Drawer*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractDrawer::Equals(const Drawer& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	return true;
}

int springai::AbstractDrawer::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);

	return internal_ret;
}

std::string springai::AbstractDrawer::ToString() {

	std::string internal_ret = "Drawer";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

