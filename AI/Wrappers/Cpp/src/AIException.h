/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _CPPWRAPPER_AI_EXCEPTION_H
#define _CPPWRAPPER_AI_EXCEPTION_H

#include <string>
#include <exception>

namespace springai {

/**
 * Common base interface for AI related Exceptions.
 */
class AIException : public std::exception {

	const int errorNumber;
	const std::string message;

public:
	AIException(int errorNumber, const std::string& message);
	~AIException() throw() {};

	/**
	 * Returns the error associated with this Exception.
	 * This is used to send over low-level language interfaces,
	 * for example C, where exceptions are not supported.
	 * @return should be != 0, as this value is reserved for the no-error state
	 */
	virtual int GetErrorNumber() const;

	virtual const char* what() const throw();
}; // class AIException

}  // namespace springai

#endif // _CPPWRAPPER_AI_EXCEPTION_H
