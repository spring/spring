/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef EXCEPTIONS_H
#define EXCEPTIONS_H

#include <stdexcept>


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


/**
 * network_error
 *   thrown when udp socket can't be bound or hostname can't be resolved
 */
class network_error : public std::runtime_error
{
public:
	network_error(const std::string& msg) : std::runtime_error(msg) {};
};


#endif
