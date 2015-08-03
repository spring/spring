/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractEconomy.h"

#include "IncludesSources.h"

	springai::AbstractEconomy::~AbstractEconomy() {}
	int springai::AbstractEconomy::CompareTo(const Economy& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((Economy*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractEconomy::Equals(const Economy& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	return true;
}

int springai::AbstractEconomy::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);

	return internal_ret;
}

std::string springai::AbstractEconomy::ToString() {

	std::string internal_ret = "Economy";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

