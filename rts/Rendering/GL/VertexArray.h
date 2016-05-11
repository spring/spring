/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H

#include "myGL.h"
#include "System/Platform/errorhandler.h"
#include "System/Color.h"
#include "System/float3.h"
#include "System/type2.h"

#define VA_INIT_VERTEXES 1000 // please don't change this, some files rely on specific initial sizes
#define VA_INIT_STRIPS 100


struct VA_TYPE_0 {
	float3 p;
};
struct VA_TYPE_N {
	float3 p;
	float3 n;
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
struct VA_TYPE_TNT {
	float3 p;
	float  s, t;
	float3 n;
	float3 uv1;
	float3 uv2;
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


// number of elements (bytes / sizeof(float)) per vertex
#define VA_SIZE_0    (sizeof(VA_TYPE_0) / sizeof(float))
#define VA_SIZE_C    (sizeof(VA_TYPE_C) / sizeof(float))
#define VA_SIZE_N    (sizeof(VA_TYPE_N) / sizeof(float))
#define VA_SIZE_T    (sizeof(VA_TYPE_T) / sizeof(float))
#define VA_SIZE_TN   (sizeof(VA_TYPE_TN) / sizeof(float))
#define VA_SIZE_TC   (sizeof(VA_TYPE_TC) / sizeof(float))
#define VA_SIZE_TNT  (sizeof(VA_TYPE_TNT) / sizeof(float))
#define VA_SIZE_2D0  (sizeof(VA_TYPE_2d0) / sizeof(float))
#define VA_SIZE_2DT  (sizeof(VA_TYPE_2dT) / sizeof(float))
#define VA_SIZE_2DTC (sizeof(VA_TYPE_2dTC) / sizeof(float))




class CVertexArray
{
public:
	typedef void (*StripCallback)(void* data);

public:
	CVertexArray(unsigned int maxVerts = 1 << 16);
	virtual ~CVertexArray();

	bool IsReady() const;
	void Initialize();
	inline void CheckInitSize(const unsigned int vertexes, const unsigned int strips = 0);
	inline void EnlargeArrays(const unsigned int vertexes, const unsigned int strips = 0, const unsigned int stripsize = VA_SIZE_0);
	unsigned int drawIndex() const { return drawArrayPos - drawArray; }
	void ResetPos() { drawArrayPos = drawArray; }

	// standard API
	inline void AddVertex0(const float3& p);
	inline void AddVertex0(float x, float y, float z);
	inline void AddVertexN(const float3& p, const float3& n);
	inline void AddVertexT(const float3& p, float tx, float ty);
	inline void AddVertexC(const float3& p, const unsigned char* c);
	inline void AddVertexTC(const float3& p, float tx, float ty, const unsigned char* c);
	inline void AddVertexTN(const float3& p, float tx, float ty, const float3& n);
	inline void AddVertexTNT(const float3& p, float tx, float ty, const float3& n, const float3& st, const float3& tt); 
	inline void AddVertex2d0(float x, float z);
	inline void AddVertex2dT(float x, float y, float tx, float ty);
	inline void AddVertex2dT(const float2 p, const float2 tc) { AddVertex2dT(p.x,p.y, tc.x,tc.y); }
	inline void AddVertex2dTC(float x, float y, float tx, float ty, const unsigned char* c);

	// same as the AddVertex... functions just without automated CheckEnlargeDrawArray
	inline void AddVertexQ0(float x, float y, float z);
	inline void AddVertexQ0(const float3& f3);
	inline void AddVertexQN(const float3& p, const float3& n);
	inline void AddVertexQC(const float3& p, const unsigned char* c);
	inline void AddVertexQT(const float3& p, float tx, float ty);
	inline void AddVertexQTN(const float3& p, float tx, float ty, const float3& n);
	inline void AddVertexQTNT(const float3& p, float tx, float ty, const float3& n, const float3& st, const float3& tt);
	inline void AddVertexQTC(const float3& p, float tx, float ty, const unsigned char* c);
	inline void AddVertexQ2d0(float x, float z);
	inline void AddVertexQ2dT(float x, float y, float tx, float ty);
	inline void AddVertexQ2dT(const float2 p, const float2 tc) { AddVertexQ2dT(p.x,p.y, tc.x,tc.y); }
	inline void AddVertexQ2dTC(float x, float y, float tx, float ty, const unsigned char* c);

	// 3rd and newest API
	// it appends a block of size * sizeof(T) at the end of the VA and returns the typed address to it
	template<typename T> T* GetTypedVertexArrayQ(const int size) {
		T* r = reinterpret_cast<T*>(drawArrayPos);
		drawArrayPos += (sizeof(T) / sizeof(float)) * size;
		return r;
	}
	template<typename T> T* GetTypedVertexArray(const int size) {
		EnlargeArrays(size, 0, (sizeof(T) / sizeof(float)));
		return GetTypedVertexArrayQ<T>(size);
	}


	// Render the VA
	void DrawArray0(const int drawType, unsigned int stride = sizeof(float) * VA_SIZE_0);
	void DrawArray2d0(const int drawType, unsigned int stride = sizeof(float) * VA_SIZE_2D0);
	void DrawArrayN(const int drawType, unsigned int stride = sizeof(float) * VA_SIZE_N);
	void DrawArrayT(const int drawType, unsigned int stride = sizeof(float) * VA_SIZE_T);
	void DrawArrayC(const int drawType, unsigned int stride = sizeof(float) * VA_SIZE_C);
	void DrawArrayTC(const int drawType, unsigned int stride = sizeof(float) * VA_SIZE_TC);
	void DrawArrayTN(const int drawType, unsigned int stride = sizeof(float) * VA_SIZE_TN);
	void DrawArrayTNT(const int drawType, unsigned int stride = sizeof(float) * VA_SIZE_TNT);
	void DrawArray2dT(const int drawType, unsigned int stride = sizeof(float) * VA_SIZE_2DT);
	void DrawArray2dTC(const int drawType, unsigned int stride = sizeof(float) * VA_SIZE_2DTC);
	void DrawArray2dT(const int drawType, StripCallback callback, void* data, unsigned int stride = sizeof(float) * VA_SIZE_2DT);

	// same as EndStrip, but without automated EnlargeStripArray
	void EndStrip();

protected:
	void DrawArrays(const GLenum mode, const unsigned int stride);
	void DrawArraysCallback(const GLenum mode, const unsigned int stride, StripCallback callback, void* data);
	inline void CheckEnlargeDrawArray();
	void EnlargeStripArray();
	void EnlargeDrawArray();
	inline void CheckEndStrip();

protected:
	float* drawArray;
	float* drawArrayPos;
	float* drawArraySize;

	unsigned int* stripArray;
	unsigned int* stripArrayPos;
	unsigned int* stripArraySize;

	unsigned int maxVertices;
};


#include "VertexArray.inl"

#endif /* VERTEXARRAY_H */
