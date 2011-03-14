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

/** @file  NullLogger.h
 *  @brief Dummy logger
*/

#ifndef INCLUDED_AI_NULLLOGGER_H
#define INCLUDED_AI_NULLLOGGER_H

#include "Logger.h"
namespace Assimp	{
// ---------------------------------------------------------------------------
/** @brief CPP-API: Empty logging implementation.
 *
 * Does nothing! Used by default if the application hasn't requested a 
 * custom logger via #DefaultLogger::set() or #DefaultLogger::create(); */
class ASSIMP_API NullLogger 
	: public Logger	{

public:

	/**	@brief	Logs a debug message */
	void OnDebug(const char* message) { 
		(void)message; //this avoids compiler warnings
	}

	/**	@brief	Logs an info message */
	void OnInfo(const char* message) { 
		(void)message; //this avoids compiler warnings
	}

	/**	@brief	Logs a warning message */
	void OnWarn(const char* message) { 
		(void)message; //this avoids compiler warnings
	}
	
	/**	@brief	Logs an error message */
	void OnError(const char* message) { 
		(void)message; //this avoids compiler warnings
	}

	/**	@brief	Detach a still attached stream from logger */
	bool attachStream(LogStream *pStream, unsigned int severity) {
		(void)pStream; (void)severity; //this avoids compiler warnings
		return false;
	}

	/**	@brief	Detach a still attached stream from logger */
	bool detatchStream(LogStream *pStream, unsigned int severity) {
		(void)pStream; (void)severity; //this avoids compiler warnings
		return false;
	}

private:
};
}
#endif // !! AI_NULLLOGGER_H_INCLUDED
