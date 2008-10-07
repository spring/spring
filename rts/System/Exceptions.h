#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>


/** @brief Compile time assertion
    @param condition Condition to test for.
    @param message Message to include in the compile error if the assert fails.
    This must be a valid C++ symbol. */
#define COMPILE_TIME_ASSERT(condition, message) \
	typedef int _compile_time_assertion_failed__ ## message [(condition) ? 1 : -1]


/**
 * content_error
 *   thrown when content couldn't be found/loaded.
 *   any other type of exception will cause a crashreport box appearing
 *     (if it is installed).
 */
class content_error : public std::runtime_error
{
public:
	content_error(const std::string& msg) : std::runtime_error(msg) {};
};

#endif
