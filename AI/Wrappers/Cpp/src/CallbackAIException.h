/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CPPWRAPPER_CALLBACK_AI_EXCEPTION_H
#define _CPPWRAPPER_CALLBACK_AI_EXCEPTION_H

#include "AIException.h"

#include <string>

class exception;

namespace springai {

/**
 * An exception of this type may be thrown from an AI callback method.
 */
class CallbackAIException : public AIException {

	const std::string methodName;

public:
	CallbackAIException(const std::string& methodName, int errorNumber, const exception* cause = NULL);
	~CallbackAIException() throw() {};

	/**
	 * Returns the name of the method in which the exception ocurred.
	 */
	virtual const std::string& GetMethodName() const;
}; // class CallbackAIException

}  // namespace springai

#endif // _CPPWRAPPER_CALLBACK_AI_EXCEPTION_H
