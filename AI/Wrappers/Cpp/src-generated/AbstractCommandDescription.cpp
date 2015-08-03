/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractCommandDescription.h"

#include "IncludesSources.h"

	springai::AbstractCommandDescription::~AbstractCommandDescription() {}
	int springai::AbstractCommandDescription::CompareTo(const CommandDescription& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((CommandDescription*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetSupportedCommandId() < other.GetSupportedCommandId()) return BEFORE;
		if (this->GetSupportedCommandId() > other.GetSupportedCommandId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractCommandDescription::Equals(const CommandDescription& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetSupportedCommandId() != other.GetSupportedCommandId()) return false;
	return true;
}

int springai::AbstractCommandDescription::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetSupportedCommandId() * (int) (10E4);

	return internal_ret;
}

std::string springai::AbstractCommandDescription::ToString() {

	std::string internal_ret = "CommandDescription";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "supportedCommandId=%i, ", this->GetSupportedCommandId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

