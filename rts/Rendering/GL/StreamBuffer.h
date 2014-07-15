/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef STREAMBUFFER_H
#define STREAMBUFFER_H

#include <vector>

#include "myGL.h"
#include "VBO.h"
#include "CircularBuffer.h"
#include "System/Color.h"

// standard VertexAttrib containers
struct VA_TYPE_0 {
	float3 p;
};
struct VA_TYPE_C {
	float3 p;
	SColor c;
};
struct VA_TYPE_T {
	float3 p;
	float  s, t;
};
struct VA_TYPE_TN {
	float3 p;
	float  s, t;
	float3 n;
};
struct VA_TYPE_TC {
	float3 p;
	float  s, t;
	SColor c;
};
struct VA_TYPE_2d0 {
	float x, y;
};
struct VA_TYPE_2dT {
	float x, y;
	float s, t;
};
struct VA_TYPE_2dTC {
	float  x, y;
	float  s, t;
	SColor c;
};

// size of the vertex attrib containers
#define VA_SIZE_0    sizeof(VA_TYPE_0)
#define VA_SIZE_C    sizeof(VA_TYPE_C)
#define VA_SIZE_T    sizeof(VA_TYPE_T)
#define VA_SIZE_TN   sizeof(VA_TYPE_TN)
#define VA_SIZE_TC   sizeof(VA_TYPE_TC)
#define VA_SIZE_2D0  sizeof(VA_TYPE_2d0)
#define VA_SIZE_2DT  sizeof(VA_TYPE_2dT)
#define VA_SIZE_2DTC sizeof(VA_TYPE_2dTC)



///////////////////////////////////////////////////////////////////////////////
/// \brief The CStreamBuffer class
///
class CStreamBuffer : public VBO, public CRangeLocker
{
public:
	typedef void (*StripCallback)(void* data);

public:
	CStreamBuffer();
	virtual ~CStreamBuffer();

	// misc
	void Initialize(const size_t stride) { Initialize(4 * 4096, stride); }
	void Initialize(size_t size, const size_t stride /*= VA_SIZE_0*/);
	void SetFence();
	size_t drawIndex() const { return curPos; }
	void ResetPos() { curPos = 0; }
	inline void EnlargeArrays(const size_t vertexes, const size_t strips, const size_t stripsize /*= VA_SIZE_0*/);

	// 1st standard API
	inline void AddVertex0(const float3& p);
	inline void AddVertex0(float x, float y, float z);
	inline void AddVertexT(const float3& p, float tx, float ty);
	inline void AddVertexC(const float3& p, const unsigned char* c);
	inline void AddVertexTC(const float3& p, float tx, float ty, const unsigned char* c);
	inline void AddVertexTN(const float3& p, float tx, float ty, const float3& n);
	inline void AddVertex2d0(float x, float z);
	inline void AddVertex2dT(float x, float y, float tx, float ty);
	inline void AddVertex2dTC(float x, float y, float tx, float ty, const unsigned char* c);

	// 2nd same as the AddVertex... functions just without automated CheckEnlargeDrawArray() calls (faster)
	inline void AddVertexQ0(const float3& f3);
	inline void AddVertexQ0(float x, float y, float z);
	inline void AddVertexQC(const float3& p, const unsigned char* c);
	inline void AddVertexQT(const float3& p, float tx, float ty);
	inline void AddVertexQTN(const float3& p, float tx, float ty, const float3& n);
	inline void AddVertexQTC(const float3& p, float tx, float ty, const unsigned char* c);
	inline void AddVertexQ2d0(float x, float z);
	inline void AddVertexQ2dT(float x, float y, float tx, float ty);
	inline void AddVertexQ2dTC(float x, float y, float tx, float ty, const unsigned char* c);

	// 3rd and newest API
	// it appends a block of size * sizeof(T) at the end of the VA and returns the typed address to it
	template<typename T> T* GetTypedVertexArrayQ(const int size) {
		T* r = reinterpret_cast<T*>(mem + bufOffset + curPos);
		curPos += sizeof(T) * size;
		return r;
	}
	template<typename T> T* GetTypedVertexArray(const int size) {
		EnlargeArrays(size, 0, sizeof(T));
		return GetTypedVertexArrayQ<T>(size);
	}


	// Render the VA
	void DrawArray0(const int drawType);
	void DrawArrayT(const int drawType);
	void DrawArrayC(const int drawType);
	void DrawArrayTC(const int drawType);
	void DrawArrayTN(const int drawType);
	void DrawArray2d0(const int drawType);
	void DrawArray2dT(const int drawType);
	void DrawArray2dTC(const int drawType);
	void DrawArray2dT(const int drawType, StripCallback callback, void* data);

	// same as EndStrip, but without automated EnlargeStripArray
	void EndStrip();


protected:
	void DrawArrays(const GLenum mode, const size_t stride) const;
	void DrawArraysCallback(const GLenum mode, const size_t stride, StripCallback callback, void* data) const;
	inline void CheckEnlargeDrawArray(const size_t spaceWanted);
	void EnlargeDrawArray(size_t size = -1);
	void CheckEndStrip();
	void GenVAOs();
	size_t GetMem(size_t size, size_t stride);
	bool EnlargeMem(size_t size);

private:
	size_t curPos;
	char* mem;
	size_t bufOffset;
	size_t bufSize;

	GLuint vao0;
	GLuint vaoC;
	GLuint vaoT;
	GLuint vaoTC;
	GLuint vaoTN;
	GLuint vao2d0;
	GLuint vao2dT;
	GLuint vao2dTC;

	std::vector<size_t> strips;
};


// backward compability
class CVertexArray : public CStreamBuffer {};


























//////////////////////////////////////////////////////////////////////
/// inlined stuff
//////////////////////////////////////////////////////////////////////

#define VA_INIT_VERTEXES (2048*1024)

void CStreamBuffer::CheckEnlargeDrawArray(const size_t spaceWanted) {
	while ((curPos + spaceWanted) > bufSize)
		EnlargeDrawArray();
}

void CStreamBuffer::EnlargeArrays(const size_t vertexes, const size_t strips, const size_t stripsize) {
	CheckEnlargeDrawArray(vertexes * stripsize);
}


//////////////////////////////////////////////////////////////////////
///
//////////////////////////////////////////////////////////////////////

#ifdef DEBUG
	#define ASSERT_SIZE(x) assert((curPos + x) <= bufSize);
#else
	#define ASSERT_SIZE(x)
#endif

void CStreamBuffer::AddVertexQ0(const float3& pos) {
	ASSERT_SIZE(VA_SIZE_0)
	VA_TYPE_0* vat = GetTypedVertexArrayQ<VA_TYPE_0>(1);
	vat->p = pos;
}

void CStreamBuffer::AddVertexQ0(float x, float y, float z) {
	ASSERT_SIZE(VA_SIZE_0)
	VA_TYPE_0* vat = GetTypedVertexArrayQ<VA_TYPE_0>(1);
	vat->p.x = x;
	vat->p.y = y;
	vat->p.z = z;
}

void CStreamBuffer::AddVertexQC(const float3& pos, const unsigned char* color) {
	ASSERT_SIZE(VA_SIZE_C)
	VA_TYPE_C* vat = GetTypedVertexArrayQ<VA_TYPE_C>(1);
	vat->p = pos;
	vat->c = SColor(color);
}

void CStreamBuffer::AddVertexQT(const float3& pos, float tx, float ty) {
	ASSERT_SIZE(VA_SIZE_T)
	VA_TYPE_T* vat = GetTypedVertexArrayQ<VA_TYPE_T>(1);
	vat->p = pos;
	vat->s = tx;
	vat->t = ty;
}

void CStreamBuffer::AddVertexQTN(const float3& pos, float tx, float ty, const float3& norm) {
	ASSERT_SIZE(VA_SIZE_TN)
	VA_TYPE_TN* vat = GetTypedVertexArrayQ<VA_TYPE_TN>(1);
	vat->p = pos;
	vat->s = tx;
	vat->t = ty;
	vat->n = norm;
}

void CStreamBuffer::AddVertexQTC(const float3& pos, float tx, float ty, const unsigned char* col) {
	ASSERT_SIZE(VA_SIZE_TC)
	VA_TYPE_TC* vat = GetTypedVertexArrayQ<VA_TYPE_TC>(1);
	vat->p = pos;
	vat->s = tx;
	vat->t = ty;
	vat->c = SColor(col);
}

void CStreamBuffer::AddVertexQ2d0(float x, float z) {
	ASSERT_SIZE(VA_SIZE_2D0)
	VA_TYPE_2d0* vat = GetTypedVertexArrayQ<VA_TYPE_2d0>(1);
	vat->x = x;
	vat->y = z;
}

void CStreamBuffer::AddVertexQ2dT(float x, float y, float tx, float ty) {
	ASSERT_SIZE(VA_SIZE_2DT)
	VA_TYPE_2dT* vat = GetTypedVertexArrayQ<VA_TYPE_2dT>(1);
	vat->x = x;
	vat->y = y;
	vat->s = tx;
	vat->t = ty;
}

void CStreamBuffer::AddVertexQ2dTC(float x, float y, float tx, float ty, const unsigned char* c) {
	ASSERT_SIZE(VA_SIZE_2DTC)
	VA_TYPE_2dTC* vat = GetTypedVertexArrayQ<VA_TYPE_2dTC>(1);
	vat->x = x;
	vat->y = y;
	vat->s = tx;
	vat->t = ty;
	vat->c = SColor(c);
}


//////////////////////////////////////////////////////////////////////
///
//////////////////////////////////////////////////////////////////////

void CStreamBuffer::AddVertex0(const float3& pos) {
	CheckEnlargeDrawArray(VA_SIZE_0);
	AddVertexQ0(pos);
}

void CStreamBuffer::AddVertex0(float x, float y, float z) {
	CheckEnlargeDrawArray(VA_SIZE_0);
	AddVertexQ0(x,y,z);
}

void CStreamBuffer::AddVertexC(const float3& pos, const unsigned char* color) {
	CheckEnlargeDrawArray(VA_SIZE_C);
	AddVertexQC(pos, color);
}

void CStreamBuffer::AddVertexT(const float3& pos, float tx, float ty) {
	CheckEnlargeDrawArray(VA_SIZE_T);
	AddVertexQT(pos, tx, ty);
}

void CStreamBuffer::AddVertexTN(const float3& pos, float tx, float ty, const float3& norm) {
	CheckEnlargeDrawArray(VA_SIZE_TN);
	AddVertexQTN(pos, tx, ty, norm);
}

void CStreamBuffer::AddVertexTC(const float3& pos, float tx, float ty, const unsigned char* col) {
	CheckEnlargeDrawArray(VA_SIZE_TC);
	AddVertexQTC(pos, tx, ty, col);
}

void CStreamBuffer::AddVertex2d0(float x, float z) {
	CheckEnlargeDrawArray(VA_SIZE_2D0);
	AddVertexQ2d0(x,z);
}

void CStreamBuffer::AddVertex2dT(float x, float y, float tx, float ty) {
	CheckEnlargeDrawArray(VA_SIZE_2DT);
	AddVertexQ2dT(x, y, tx, ty);
}

void CStreamBuffer::AddVertex2dTC(float x, float y, float tx, float ty, const unsigned char* col) {
	CheckEnlargeDrawArray(VA_SIZE_2DTC);
	AddVertexQ2dTC(x, y, tx, ty, col);
}


#endif /* STREAMBUFFER_H */
