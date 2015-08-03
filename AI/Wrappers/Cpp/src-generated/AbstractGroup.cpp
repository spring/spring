/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractGroup.h"

#include "IncludesSources.h"

	springai::AbstractGroup::~AbstractGroup() {}
	int springai::AbstractGroup::CompareTo(const Group& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((Group*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetGroupId() < other.GetGroupId()) return BEFORE;
		if (this->GetGroupId() > other.GetGroupId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractGroup::Equals(const Group& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetGroupId() != other.GetGroupId()) return false;
	return true;
}

int springai::AbstractGroup::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetGroupId() * (int) (10E5);

	return internal_ret;
}

std::string springai::AbstractGroup::ToString() {

	std::string internal_ret = "Group";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "groupId=%i, ", this->GetGroupId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

