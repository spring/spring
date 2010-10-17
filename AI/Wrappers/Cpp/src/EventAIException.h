/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CPPWRAPPER_EVENT_AI_EXCEPTION_H
#define _CPPWRAPPER_EVENT_AI_EXCEPTION_H

#include "AIException.h"

#include <string>

namespace springai {

/**
 * An exception of this type may be thrown while handling an AI event.
 */
class EventAIException : public AIException {
public:
	static const int DEFAULT_ERROR_NUMBER = 10;

	EventAIException(const std::string& message, int errorNumber = DEFAULT_ERROR_NUMBER);
	~EventAIException() throw() {};
}; // class EventAIException

}  // namespace springai

#endif // _CPPWRAPPER_EVENT_AI_EXCEPTION_H
