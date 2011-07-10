/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2010, ASSIMP Development Team

All rights reserved.

Redistribution and use of this software in source and binary forms, 
with or without modification, are permitted provided that the following 
conditions are met:

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
---------------------------------------------------------------------------
*/

/**  @file  qnan.h
 *   @brief Some utilities for our dealings with qnans.
 *
 *   @note Some loaders use qnans to mark invalid values tempoarily, also
 *     Assimp explicitly enforces undefined normals to be set to qnan.
 *     qnan utilities are available in standard libraries (C99 for example)
 *     but last time I checked compiler coverage was so bad that I decided
 *     to reinvent the wheel.
 */

#ifndef AI_QNAN_H_INCLUDED
#define AI_QNAN_H_INCLUDED

// ---------------------------------------------------------------------------
/** Data structure to represent the bit pattern of a 32 Bit 
 *         IEEE 754 floating-point number. */
union _IEEESingle
{
	float Float;
	struct
	{
		uint32_t Frac : 23;
		uint32_t Exp  : 8;
		uint32_t Sign : 1;
	} IEEE;
} ;

// ---------------------------------------------------------------------------
/** Check whether a given float is qNaN. 
 *  @param in Input value */
AI_FORCE_INLINE bool is_qnan(float in)	
{
	// the straightforward solution does not work:
	//   return (in != in);
	// compiler generates code like this
	//   load <in> to <register-with-different-width>
	//   compare <register-with-different-width> against <in>

	// FIXME: Use <float> stuff instead? I think fpclassify needs C99
	return (reinterpret_cast<_IEEESingle*>(&in)->IEEE.Exp == (1u << 8)-1 &&
		reinterpret_cast<_IEEESingle*>(&in)->IEEE.Frac);
}

// ---------------------------------------------------------------------------
/** Check whether a float is NOT qNaN. 
 *  @param in Input value */
AI_FORCE_INLINE bool is_not_qnan(float in)	
{
	return !is_qnan(in);
}

// ---------------------------------------------------------------------------
/** @brief check whether a float is either NaN or (+/-) INF. 
 *
 *  Denorms return false, they're treated like normal values.
 *  @param in Input value */
AI_FORCE_INLINE bool is_special_float(float in)
{
	return (reinterpret_cast<_IEEESingle*>(&in)->IEEE.Exp == (1u << 8)-1);
}

// ---------------------------------------------------------------------------
/** @brief Get a fresh qnan.  */
AI_FORCE_INLINE float get_qnan()
{
	return std::numeric_limits<float>::quiet_NaN();
}

#endif // !! AI_QNAN_H_INCLUDED
