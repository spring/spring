/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H

#include "myGL.h"
#include "Platform/errorhandler.h"

#define VA_INIT_VERTEXES 1000 // please don't change this, some files rely on specific initial sizes
#define VA_INIT_STRIPS 100

// number of elements (bytes / 4) per vertex
#define VA_SIZE_2D0  2
#define VA_SIZE_0    3
#define VA_SIZE_C    4
#define VA_SIZE_T    5
#define VA_SIZE_TN   8
#define VA_SIZE_TNT 14
#define VA_SIZE_TC   6
#define VA_SIZE_2DT  4

class CVertexArray  
{
public:
	typedef void (*StripCallback)(void* data);

public:
	CVertexArray(unsigned int maxVerts = 1 << 16);
	~CVertexArray();
	void Initialize();
	inline void CheckInitSize(const unsigned int vertexes, const unsigned int strips = 0);

	inline void AddVertex0(const float3& p);
	inline void AddVertex0(float x, float y, float z);
	inline void AddVertex2d0(float x, float z);
	inline void AddVertexN(const float3& p, const float3& normal);
	inline void AddVertexT(const float3& p, float tx, float ty);
	inline void AddVertexC(const float3& p, const unsigned char* color);
	inline void AddVertexTC(const float3& p, float tx, float ty, const unsigned char* color);
	inline void AddVertexTN(const float3& p, float tx, float ty, const float3& n);
	inline void AddVertexTNT(const float3& p, float tx, float ty, const float3& n, const float3& st, const float3& tt); 
	inline void AddVertexT2(const float3& p, float t1x, float t1y, float t2x, float t2y);
	inline void AddVertex2dT(float x, float y, float tx, float ty);

	void DrawArray0(const int drawType, unsigned int stride = 12);
	void DrawArray2d0(const int drawType, unsigned int stride = 8);
	void DrawArrayN(const int drawType, unsigned int stride = 24);
	void DrawArrayT(const int drawType, unsigned int stride = 20);
	void DrawArrayC(const int drawType, unsigned int stride = 16);
	void DrawArrayTC(const int drawType, unsigned int stride = 24);
	void DrawArrayTN(const int drawType, unsigned int stride = 32);
	void DrawArrayTNT(const int drawType, unsigned int stride = 56);
	void DrawArrayT2(const int drawType, unsigned int stride = 28);
	void DrawArray2dT(const int drawType, unsigned int stride = 16);
	void DrawArray2dT(const int drawType, StripCallback callback, void* data, unsigned int stride=16);

	//! same as the AddVertex... functions just without automated CheckEnlargeDrawArray
	inline void AddVertexQ0(float x, float y, float z);
	inline void AddVertexQ0(const float3& f3) { AddVertexQ0(f3.x, f3.y, f3.z); }
	inline void AddVertex2dQ0(float x, float z);
	inline void AddVertexQN(const float3& p, const float3& n);
	inline void AddVertexQC(const float3& p, const unsigned char* color);
	inline void AddVertexQT(const float3& p, float tx, float ty);
	inline void AddVertex2dQT(float x, float y, float tx, float ty);
	inline void AddVertexQTN(const float3& p, float tx, float ty, const float3& n);
	inline void AddVertexQTNT(const float3& p, float tx, float ty, const float3& n, const float3& st, const float3& tt);
	inline void AddVertexQTC(const float3& p, float tx, float ty, const unsigned char* color);

	//! same as EndStrip, but without automated EnlargeStripArray
	inline void EndStripQ();

	void EndStrip();

	bool IsReady() const;
	inline unsigned int drawIndex() const;
	inline void EnlargeArrays(const unsigned int vertexes, const unsigned int strips, const unsigned int stripsize=VA_SIZE_0);

	float* drawArray;
	float* drawArrayPos;
	float* drawArraySize;

	unsigned int* stripArray;
	unsigned int* stripArrayPos;
	unsigned int* stripArraySize;

	unsigned int maxVertices;

protected:
	void DrawArrays(const GLenum mode, const unsigned int stride);
	void DrawArraysCallback(const GLenum mode, const unsigned int stride, StripCallback callback, void* data);
	inline void CheckEnlargeDrawArray();
	void EnlargeStripArray();
	void EnlargeDrawArray();
	inline void CheckEndStrip();
};

#include "VertexArray.inl"

#endif /* VERTEXARRAY_H */
