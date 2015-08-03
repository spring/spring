/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractMoveData.h"

#include "IncludesSources.h"

	springai::AbstractMoveData::~AbstractMoveData() {}
	int springai::AbstractMoveData::CompareTo(const MoveData& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((MoveData*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetUnitDefId() < other.GetUnitDefId()) return BEFORE;
		if (this->GetUnitDefId() > other.GetUnitDefId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractMoveData::Equals(const MoveData& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetUnitDefId() != other.GetUnitDefId()) return false;
	return true;
}

int springai::AbstractMoveData::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetUnitDefId() * (int) (10E5);

	return internal_ret;
}

std::string springai::AbstractMoveData::ToString() {

	std::string internal_ret = "MoveData";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "unitDefId=%i, ", this->GetUnitDefId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

