/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractUnitRulesParam.h"

#include "IncludesSources.h"

	springai::AbstractUnitRulesParam::~AbstractUnitRulesParam() {}
	int springai::AbstractUnitRulesParam::CompareTo(const UnitRulesParam& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((UnitRulesParam*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetUnitId() < other.GetUnitId()) return BEFORE;
		if (this->GetUnitId() > other.GetUnitId()) return AFTER;
		if (this->GetUnitRulesParamId() < other.GetUnitRulesParamId()) return BEFORE;
		if (this->GetUnitRulesParamId() > other.GetUnitRulesParamId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractUnitRulesParam::Equals(const UnitRulesParam& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetUnitId() != other.GetUnitId()) return false;
	if (this->GetUnitRulesParamId() != other.GetUnitRulesParamId()) return false;
	return true;
}

int springai::AbstractUnitRulesParam::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetUnitId() * (int) (10E5);
	internal_ret += this->GetUnitRulesParamId() * (int) (10E4);

	return internal_ret;
}

std::string springai::AbstractUnitRulesParam::ToString() {

	std::string internal_ret = "UnitRulesParam";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "unitId=%i, ", this->GetUnitId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "unitRulesParamId=%i, ", this->GetUnitRulesParamId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

