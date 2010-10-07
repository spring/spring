/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "EventAIException.h"

#include "AIException.h"

#include <string>

//static const int springai::EventAIException::DEFAULT_ERROR_NUMBER = 10;

springai::EventAIException::EventAIException(const std::string& message, int errorNumber)
	: AIException(errorNumber, message)
{
}
