/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CallbackAIException.h"

#include "AIException.h"

#include "System/StringUtil.h"

#include <string>

springai::CallbackAIException::CallbackAIException(const std::string& methodName, int errorNumber, const exception* cause)
	: AIException(errorNumber, "Error calling method \"" + methodName + "\": " + IntToString(errorNumber))
	, methodName(methodName)
{
}

const std::string& springai::CallbackAIException::GetMethodName() const {
	return methodName;
}
