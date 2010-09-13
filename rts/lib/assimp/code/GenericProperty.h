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

#ifndef AI_GENERIC_PROPERTY_H_INCLUDED
#define AI_GENERIC_PROPERTY_H_INCLUDED

#include "./../include/assimp.hpp"
#include "Hash.h"

// ------------------------------------------------------------------------------------------------
template <class T>
inline void SetGenericProperty(std::map< unsigned int, T >& list, 
	const char* szName, const T& value, bool* bWasExisting = NULL)
{
	ai_assert(NULL != szName);
	const uint32_t hash = SuperFastHash(szName);

	typename std::map<unsigned int, T>::iterator it = list.find(hash);
	if (it == list.end())	{
		if (bWasExisting)
			*bWasExisting = false;
		list.insert(std::pair<unsigned int, T>( hash, value ));
		return;
	}
	(*it).second = value;
	if (bWasExisting)
		*bWasExisting = true;
}

// ------------------------------------------------------------------------------------------------
template <class T>
inline const T& GetGenericProperty(const std::map< unsigned int, T >& list, 
	const char* szName, const T& errorReturn)
{
	ai_assert(NULL != szName);
	const uint32_t hash = SuperFastHash(szName);

	typename std::map<unsigned int, T>::const_iterator it = list.find(hash);
	if (it == list.end())
		return errorReturn;
	
	return (*it).second;
}

// ------------------------------------------------------------------------------------------------
// Special version for pointer types - they will be deleted when replaced with another value
// passing NULL removes the whole property
template <class T>
inline void SetGenericPropertyPtr(std::map< unsigned int, T* >& list, 
	const char* szName, T* value, bool* bWasExisting = NULL)
{
	ai_assert(NULL != szName);
	const uint32_t hash = SuperFastHash(szName);

	typename std::map<unsigned int, T*>::iterator it = list.find(hash);
	if (it == list.end())	{
		if (bWasExisting)
			*bWasExisting = false;
		
		list.insert(std::pair<unsigned int,T*>( hash, value ));
		return;
	}
	if ((*it).second != value)	{
		delete (*it).second;
		(*it).second = value;
	}
	if (!value)	{
		list.erase(it);
	}
	if (bWasExisting)
		*bWasExisting = true;
}


#endif // !! AI_GENERIC_PROPERTY_H_INCLUDED
