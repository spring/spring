/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "AIException.h"

#include <string>

springai::AIException::AIException(int errorNumber, const std::string& message)
	: errorNumber(errorNumber)
	, message(message)
{
}

int springai::AIException::GetErrorNumber() const {
	return errorNumber;
}

const char* springai::AIException::what() const throw() {
	return message.c_str();
}
