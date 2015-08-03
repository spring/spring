/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractGameRulesParam.h"

#include "IncludesSources.h"

	springai::AbstractGameRulesParam::~AbstractGameRulesParam() {}
	int springai::AbstractGameRulesParam::CompareTo(const GameRulesParam& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((GameRulesParam*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetGameRulesParamId() < other.GetGameRulesParamId()) return BEFORE;
		if (this->GetGameRulesParamId() > other.GetGameRulesParamId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractGameRulesParam::Equals(const GameRulesParam& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetGameRulesParamId() != other.GetGameRulesParamId()) return false;
	return true;
}

int springai::AbstractGameRulesParam::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetGameRulesParamId() * (int) (10E5);

	return internal_ret;
}

std::string springai::AbstractGameRulesParam::ToString() {

	std::string internal_ret = "GameRulesParam";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "gameRulesParamId=%i, ", this->GetGameRulesParamId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

