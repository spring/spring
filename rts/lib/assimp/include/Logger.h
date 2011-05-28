/*
Open Asset Import Library (ASSIMP)
----------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team
All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the 
following conditions are met:

* Redistributions of source code must retain the above
  copyright notice, this list of conditions and the
  following disclaimer.

* Redistributions in binary form must reproduce the above
  copyright notice, this list of conditions and the
  following disclaimer in the documentation and/or other
  materials provided with the distribution.

* Neither the name of the ASSIMP team, nor the names of its
  contributors may be used to endorse or promote products
  derived from this software without specific prior
  written permission of the ASSIMP Development Team.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

----------------------------------------------------------------------
*/

/** @file Logger.h
 *  @brief Abstract base class 'Logger', base of the logging system. 
 */
#ifndef INCLUDED_AI_LOGGER_H
#define INCLUDED_AI_LOGGER_H

#include "aiTypes.h"
namespace Assimp	{
class LogStream;

// Maximum length of a log message. Longer messages are rejected.
#define MAX_LOG_MESSAGE_LENGTH 1024u

// ----------------------------------------------------------------------------------
/**	@brief CPP-API: Abstract interface for logger implementations.
 *  Assimp provides a default implementation and uses it for almost all 
 *  logging stuff ('DefaultLogger'). This class defines just basic logging
 *  behaviour and is not of interest for you. Instead, take a look at #DefaultLogger. */
class ASSIMP_API Logger 
	: public Intern::AllocateFromAssimpHeap	{
public:

	// ----------------------------------------------------------------------
	/**	@enum	LogSeverity
	 *	@brief	Log severity to describe the granularity of logging.
	 */
	enum LogSeverity
	{
		NORMAL,		//!< Normal granularity of logging
		VERBOSE		//!< Debug infos will be logged, too
	};

	// ----------------------------------------------------------------------
	/**	@enum	ErrorSeverity
	 *	@brief	Description for severity of a log message.
	 *
	 *  Every LogStream has a bitwise combination of these flags.
	 *  A LogStream doesn't receive any messages of a specific type
	 *  if it doesn't specify the corresponding ErrorSeverity flag.
	 */
	enum ErrorSeverity
	{
		DEBUGGING	= 1,	//!< Debug log message
		INFO		= 2, 	//!< Info log message
		WARN		= 4,	//!< Warn log message
		ERR			= 8		//!< Error log message
	};

public:

	/** @brief	Virtual destructor */
	virtual ~Logger();

	// ----------------------------------------------------------------------
	/** @brief	Writes a debug message
	 *	 @param	message	Debug message*/
	void debug(const std::string &message);

	// ----------------------------------------------------------------------
	/** @brief	Writes a info message
	 *	@param	message Info message*/
	void info(const std::string &message);

	// ----------------------------------------------------------------------
	/** @brief	Writes a warning message
	 *	@param	message Warn message*/
	void warn(const std::string &message);

	// ----------------------------------------------------------------------
	/** @brief	Writes an error message
	 *	@param	message	Error message*/
	void error(const std::string &message);

	// ----------------------------------------------------------------------
	/** @brief	Set a new log severity.
	 *	@param	log_severity New severity for logging*/
	void setLogSeverity(LogSeverity log_severity);

	// ----------------------------------------------------------------------
	/** @brief Get the current log severity*/
	LogSeverity getLogSeverity() const;

	// ----------------------------------------------------------------------
	/** @brief	Attach a new logstream
	 *
	 *  The logger takes ownership of the stream and is responsible
	 *  for its destruction (which is done using ::delete when the logger
	 *  itself is destroyed). Call detachStream to detach a stream and to
	 *  gain ownership of it again.
	 *	 @param	pStream	 Logstream to attach
	 *  @param severity  Message filter, specified which types of log
	 *    messages are dispatched to the stream. Provide a bitwise
	 *    combination of the ErrorSeverity flags.
	 *  @return true if the stream has been attached, false otherwise.*/
	virtual bool attachStream(LogStream *pStream, 
		unsigned int severity = DEBUGGING | ERR | WARN | INFO) = 0;

	// ----------------------------------------------------------------------
	/** @brief	Detach a still attached stream from the logger (or 
	 *          modify the filter flags bits)
	 *	 @param	pStream	Logstream instance for detaching
	 *  @param severity Provide a bitwise combination of the ErrorSeverity
	 *    flags. This value is &~ed with the current flags of the stream,
	 *    if the result is 0 the stream is detached from the Logger and
	 *    the caller retakes the possession of the stream.
	 *  @return true if the stream has been dettached, false otherwise.*/
	virtual bool detatchStream(LogStream *pStream, 
		unsigned int severity = DEBUGGING | ERR | WARN | INFO) = 0;

protected:

	/** Default constructor */
	Logger();

	/** Construction with a given log severity */
	Logger(LogSeverity severity);

	// ----------------------------------------------------------------------
	/** @brief Called as a request to write a specific debug message
	 *	@param	message	Debug message. Never longer than
	 *    MAX_LOG_MESSAGE_LENGTH characters (exluding the '0').
	 *  @note  The message string is only valid until the scope of
	 *    the function is left.
	 */
	virtual void OnDebug(const char* message)= 0;

	// ----------------------------------------------------------------------
	/** @brief Called as a request to write a specific info message
	 *	@param	message	Info message. Never longer than
	 *    MAX_LOG_MESSAGE_LENGTH characters (exluding the '0').
	 *  @note  The message string is only valid until the scope of
	 *    the function is left.
	 */
	virtual void OnInfo(const char* message) = 0;

	// ----------------------------------------------------------------------
	/** @brief Called as a request to write a specific warn message
	 *	@param	message	Warn message. Never longer than
	 *    MAX_LOG_MESSAGE_LENGTH characters (exluding the '0').
	 *  @note  The message string is only valid until the scope of
	 *    the function is left.
	 */
	virtual void OnWarn(const char* essage) = 0;

	// ----------------------------------------------------------------------
	/** @brief Called as a request to write a specific error message
	 *	@param	message Error message. Never longer than
	 *    MAX_LOG_MESSAGE_LENGTH characters (exluding the '0').
	 *  @note  The message string is only valid until the scope of
	 *    the function is left.
	 */
	virtual void OnError(const char* message) = 0;

protected:

	//!	Logger severity
	LogSeverity m_Severity;
};

// ----------------------------------------------------------------------------------
//	Default constructor
inline Logger::Logger()	{
	setLogSeverity(NORMAL);
}

// ----------------------------------------------------------------------------------
//	Virtual destructor
inline  Logger::~Logger()
{
}

// ----------------------------------------------------------------------------------
// Construction with given logging severity
inline Logger::Logger(LogSeverity severity)	{
	setLogSeverity(severity);
}

// ----------------------------------------------------------------------------------
// Log severity setter
inline void Logger::setLogSeverity(LogSeverity log_severity){
	m_Severity = log_severity;
}

// ----------------------------------------------------------------------------------
// Log severity getter
inline Logger::LogSeverity Logger::getLogSeverity() const {
	return m_Severity;
}

// ----------------------------------------------------------------------------------

} // Namespace Assimp

#endif // !! INCLUDED_AI_LOGGER_H
