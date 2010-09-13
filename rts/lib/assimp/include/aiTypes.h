/*
---------------------------------------------------------------------------
Open Asset Import Library (ASSIMP)
---------------------------------------------------------------------------

Copyright (c) 2006-2008, ASSIMP Development Team

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

/** @file aiTypes.h
*/

#ifndef AI_TYPES_H_INC
#define AI_TYPES_H_INC

// Some CRT headers
#include <sys/types.h>
#include <memory.h>
#include <math.h>
#include <stddef.h>

// Our compile configuration
#include "aiDefines.h"

// Some types moved to separate header due to size of operators
#include "aiVector3D.h"
#include "aiVector2D.h"
#include "aiMatrix3x3.h"
#include "aiMatrix4x4.h"
#include "aiQuaternion.h"

#ifdef __cplusplus
#	include <string> // for aiString::Set(const std::string&)

namespace Assimp	{
namespace Intern		{

	/** @brief Internal helper class to utilize our internal new/delete 
	 *    routines for allocating object of this and derived classes.
	 *
	 * By doing this you can safely share class objects between Assimp
	 * and the application - it works even over DLL boundaries. A good
	 * example is the IOSystem where the application allocates its custom
	 * IOSystem, then calls Importer::SetIOSystem(). When the Importer
	 * destructs, Assimp calls operator delete on the stored IOSystem.
	 * If it lies on a different heap than Assimp is working with,
	 * the application is determined to crash.
	 */
	struct ASSIMP_API AllocateFromAssimpHeap	{

		// new/delete overload
		void *operator new    ( size_t num_bytes);
		void  operator delete ( void* data);

		// array new/delete overload
		void *operator new[]    ( size_t num_bytes);
		void  operator delete[] ( void* data);

	}; //! struct AllocateFromAssimpHeap
} //! namespace Intern
} //! namespace Assimp

extern "C" {
#endif

/** Maximum dimension for strings, ASSIMP strings are zero terminated. */
#ifdef __cplusplus
const size_t MAXLEN = 1024;
#else
#	define MAXLEN 1024
#endif

#include "./Compiler/pushpack1.h"

// ----------------------------------------------------------------------------------
/** Represents a plane in a three-dimensional, euclidean space
*/
struct aiPlane
{
#ifdef __cplusplus
	aiPlane () : a(0.f), b(0.f), c(0.f), d(0.f) {}
	aiPlane (float _a, float _b, float _c, float _d) 
		: a(_a), b(_b), c(_c), d(_d) {}

	aiPlane (const aiPlane& o) : a(o.a), b(o.b), c(o.c), d(o.d) {}

#endif // !__cplusplus

	//! Plane equation
	float a,b,c,d;
} PACK_STRUCT; // !struct aiPlane


// ----------------------------------------------------------------------------------
/** Represents a ray
*/
struct aiRay
{
#ifdef __cplusplus
	aiRay () {}
	aiRay (const aiVector3D& _pos, const aiVector3D& _dir)
		: pos(_pos), dir(_dir) {}

	aiRay (const aiRay& o) : pos (o.pos), dir (o.dir) {}

#endif // !__cplusplus

	//! Position and direction of the ray
	C_STRUCT aiVector3D pos, dir;
} PACK_STRUCT; // !struct aiRay



// ----------------------------------------------------------------------------------
/** Represents a color in Red-Green-Blue space. 
*/
struct aiColor3D
{
#ifdef __cplusplus
	aiColor3D () : r(0.0f), g(0.0f), b(0.0f) {}
	aiColor3D (float _r, float _g, float _b) : r(_r), g(_g), b(_b) {}
	aiColor3D (const aiColor3D& o) : r(o.r), g(o.g), b(o.b) {}
	
	// Component-wise comparison 
	// TODO: add epsilon?
	bool operator == (const aiColor3D& other) const
		{return r == other.r && g == other.g && b == other.b;}

	// Component-wise inverse comparison 
	// TODO: add epsilon?
	bool operator != (const aiColor3D& other) const
		{return r != other.r || g != other.g || b != other.b;}

	// Component-wise addition
	aiColor3D operator+(const aiColor3D& c) const {
		return aiColor3D(r+c.r,g+c.g,b+c.b);
	}

	// Component-wise subtraction
	aiColor3D operator-(const aiColor3D& c) const {
		return aiColor3D(r+c.r,g+c.g,b+c.b);
	}

	// Component-wise multiplication
	aiColor3D operator*(const aiColor3D& c) const {
		return aiColor3D(r*c.r,g*c.g,b*c.b);
	}
	
	// Multiply with a scalar
	aiColor3D operator*(float f) const {
		return aiColor3D(r*f,g*f,b*f);
	}

	// Access a specific color component
	float operator[](unsigned int i) const {return *(&r + i);}
	float& operator[](unsigned int i) {return *(&r + i);}

	// Check whether a color is black
	bool IsBlack() const
	{
		static const float epsilon = 10e-3f;
		return fabs( r ) < epsilon && fabs( g ) < epsilon && fabs( b ) < epsilon;
	}

#endif // !__cplusplus

	//! Red, green and blue color values
	float r, g, b;
} PACK_STRUCT;  // !struct aiColor3D


// ----------------------------------------------------------------------------------
/** Represents a color in Red-Green-Blue space including an 
*   alpha component. 
*/
struct aiColor4D
{
#ifdef __cplusplus
	aiColor4D () : r(0.0f), g(0.0f), b(0.0f), a(0.0f) {}
	aiColor4D (float _r, float _g, float _b, float _a) 
		: r(_r), g(_g), b(_b), a(_a) {}
	aiColor4D (const aiColor4D& o) 
		: r(o.r), g(o.g), b(o.b), a(o.a) {}
	
	// Component-wise comparison 
	// TODO: add epsilon?
	bool operator == (const aiColor4D& other) const {
		return r == other.r && g == other.g && b == other.b && a == other.a;
	}

	// Component-wise inverse comparison 
	// TODO: add epsilon?
	bool operator != (const aiColor4D& other) const {
		return r != other.r || g != other.g || b != other.b || a != other.a;
	}

	// Access a specific color component
	inline float operator[](unsigned int i) const {return *(&r + i);}
	inline float& operator[](unsigned int i) {return *(&r + i);}

	// Check whether a color is black
	// TODO: add epsilon?
	inline bool IsBlack() const
	{
		// The alpha component doesn't care here. black is black.
		static const float epsilon = 10e-3f;
		return fabs( r ) < epsilon && fabs( g ) < epsilon && fabs( b ) < epsilon;
	}

#endif // !__cplusplus

	//! Red, green, blue and alpha color values
	float r, g, b, a;
} PACK_STRUCT;  // !struct aiColor4D


#include "./Compiler/poppack1.h"


// ----------------------------------------------------------------------------------
/** Represents a string, zero byte terminated.
 *
 *  We use this representation to be C-compatible. The length of such a string is
 *  limited to MAXLEN characters (excluding the terminal zero).
*/
struct aiString
{
#ifdef __cplusplus

	//! Default constructor, the string is set to have zero length
	aiString() :
		length(0) 
	{
		data[0] = '\0';

#ifdef _DEBUG
		// Debug build: overwrite the string on its full length with ESC (27)
		::memset(data+1,27,MAXLEN-1);
#endif
	}

	//! Copy constructor
	aiString(const aiString& rOther) : 
		length(rOther.length) 
	{
		::memcpy( data, rOther.data, rOther.length);
		data[length] = '\0';
	}

	//! Constructor from std::string
	aiString(const std::string& pString) : 
		length(pString.length()) 
	{
		memcpy( data, pString.c_str(), length);
		data[length] = '\0';
	}

	//! Copy a std::string to the aiString
	void Set( const std::string& pString)
	{
		if( pString.length() > MAXLEN - 1)
			return;
		length = pString.length();
		::memcpy( data, pString.c_str(), length);
		data[length] = 0;
	}

	//! Copy a const char* to the aiString
	void Set( const char* sz)
	{
		const size_t len = ::strlen(sz);
		if( len > MAXLEN - 1)
			return;
		length = len;
		::memcpy( data, sz, len);
		data[len] = 0;
	}

	// Assign a const char* to the string
	aiString& operator = (const char* sz)
	{
		Set(sz);
		return *this;
	}

	// Assign a cstd::string to the string
	aiString& operator = ( const std::string& pString)
	{
		Set(pString);
		return *this;
	}

	//! Comparison operator
	bool operator==(const aiString& other) const
	{
		return  (length == other.length && 0 == strcmp(this->data,other.data));
	}

	//! Inverse comparison operator
	bool operator!=(const aiString& other) const
	{
		return  (length != other.length || 0 != ::strcmp(this->data,other.data));
	}

	//! Append a string to the string
	void Append (const char* app)
	{
		const size_t len = ::strlen(app);
		if (!len)return;

		if (length + len >= MAXLEN)
			return;

		::memcpy(&data[length],app,len+1);
		length += len;
	}

	//! Clear the string - reset its length to zero
	void Clear ()
	{
		length  = 0;
		data[0] = '\0';

#ifdef _DEBUG
		// Debug build: overwrite the string on its full length with ESC (27)
		::memset(data+1,27,MAXLEN-1);
#endif
	}

#endif // !__cplusplus

	//! Length of the string excluding the terminal 0
	size_t length;

	//! String buffer. Size limit is MAXLEN
	char data[MAXLEN];
} ;  // !struct aiString


// ----------------------------------------------------------------------------------
/**	Standard return type for all library functions.
*
* To check whether or not a function failed check against
* AI_SUCCESS. The error codes are mainly used by the C-API.
*/
enum aiReturn
{
	//! Indicates that a function was successful
	AI_SUCCESS = 0x0,

	//! Indicates that a function failed
	AI_FAILURE = -0x1,

	//! Indicates that a file was invalid
	AI_INVALIDFILE = -0x2,

	//! Indicates that not enough memory was available
	//! to perform the requested operation
	AI_OUTOFMEMORY = -0x3,

	//! Indicates that an illegal argument has been
	//! passed to a function. This is rarely used,
	//! most functions assert in this case.
	AI_INVALIDARG = -0x4,

	//! Force 32-bit size enum 
	_AI_ENFORCE_ENUM_SIZE = 0x7fffffff 
};  // !enum aiReturn


// ----------------------------------------------------------------------------------
/** Seek origins (for the virtual file system API)
*/
enum aiOrigin
{
	//! Beginning of the file
	aiOrigin_SET = 0x0,	

	//! Current position of the file pointer
	aiOrigin_CUR = 0x1,		

	//! End of the file, offsets must be negative
	aiOrigin_END = 0x2,

	//! Force 32-bit size enum 
	_AI_ORIGIN_ENFORCE_ENUM_SIZE = 0x7fffffff 
}; // !enum aiOrigin


// ----------------------------------------------------------------------------------
/** Stores the memory requirements for different parts (e.g. meshes, materials,
 *  animations) of an import.
 *  @see Importer::GetMemoryRequirements()
*/
struct aiMemoryInfo
{
#ifdef __cplusplus

	//! Default constructor
	aiMemoryInfo()
		: textures   (0)
		, materials  (0)
		, meshes     (0)
		, nodes      (0)
		, animations (0)
		, cameras	 (0)
		, lights	 (0)
		, total      (0)
	{}

#endif

	//! Storage allocated for texture data, in bytes
	unsigned int textures;

	//! Storage allocated for material data, in bytes
	unsigned int materials;

	//! Storage allocated for mesh data, in bytes
	unsigned int meshes;

	//! Storage allocated for node data, in bytes
	unsigned int nodes;

	//! Storage allocated for animation data, in bytes
	unsigned int animations;

	//! Storage allocated for camera data, in bytes
	unsigned int cameras;

	//! Storage allocated for light data, in bytes
	unsigned int lights;

	//! Storage allocated for the full import, in bytes
	unsigned int total;
}; // !struct aiMemoryInfo 


#ifdef __cplusplus
}
#endif //!  __cplusplus

// Include implementations
#include "aiVector3D.inl"
#include "aiMatrix3x3.inl"
#include "aiMatrix4x4.inl"



#endif //!! include guard

