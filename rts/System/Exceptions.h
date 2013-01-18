/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

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
 * user_error
 *   thrown when a enduser config is broken/invalid.
 */
class user_error : public std::runtime_error
{
public:
	user_error(const std::string& msg) : std::runtime_error(msg) {};
};


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


/**
 * opengl_error
 *   thrown when an OpenGL function failed (FBO creation, Offscreen Context creation, ...).
 */
class opengl_error : public std::runtime_error
{
public:
	opengl_error(const std::string& msg) : std::runtime_error(msg) {};
};


/**
 * unsupported_error
 *   thrown when code cannot be executed cause the system is unsupported (GPU is missing extensions etc.)
 */
class unsupported_error : public std::runtime_error
{
public:
	unsupported_error(const std::string& msg) : std::runtime_error(msg) {};
};

#endif
