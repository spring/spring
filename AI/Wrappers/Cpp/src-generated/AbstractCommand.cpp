/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/* Note: This file is machine generated, do not edit directly! */

#include "AbstractCommand.h"

#include "IncludesSources.h"

	springai::AbstractCommand::~AbstractCommand() {}
	int springai::AbstractCommand::CompareTo(const Command& other) {
		static const int EQUAL  =  0;
		static const int BEFORE = -1;
		static const int AFTER  =  1;

		if ((Command*)this == &other) return EQUAL;

		if (this->GetSkirmishAIId() < other.GetSkirmishAIId()) return BEFORE;
		if (this->GetSkirmishAIId() > other.GetSkirmishAIId()) return AFTER;
		if (this->GetCommandId() < other.GetCommandId()) return BEFORE;
		if (this->GetCommandId() > other.GetCommandId()) return AFTER;
		return EQUAL;
	}

bool springai::AbstractCommand::Equals(const Command& other) {

	if (this->GetSkirmishAIId() != other.GetSkirmishAIId()) return false;
	if (this->GetCommandId() != other.GetCommandId()) return false;
	return true;
}

int springai::AbstractCommand::HashCode() {

	int internal_ret = 23;

	internal_ret += this->GetSkirmishAIId() * (int) (10E6);
	internal_ret += this->GetCommandId() * (int) (10E4);

	return internal_ret;
}

std::string springai::AbstractCommand::ToString() {

	std::string internal_ret = "Command";

	char _buff[64];
	sprintf(_buff, "skirmishAIId=%i, ", this->GetSkirmishAIId());
	internal_ret = internal_ret + _buff;
	sprintf(_buff, "commandId=%i, ", this->GetCommandId());
	internal_ret = internal_ret + _buff;
	internal_ret = internal_ret + ")";

	return internal_ret;
}

