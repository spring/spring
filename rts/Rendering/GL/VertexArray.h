/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H

#include "myGL.h"
#include "VertexArrayTypes.h"
#include "System/Platform/errorhandler.h"
#include "System/Color.h"
#include "System/float3.h"
#include "System/type2.h"

constexpr size_t VA_INIT_VERTEXES = 1000; // please don't change this, some files rely on specific initial sizes
constexpr size_t VA_INIT_STRIPS = 100;

class CVertexArray
{
public:
	typedef void (*StripCallback)(void* data);

public:
	CVertexArray(unsigned int maxVerts = 1 << 16);
	CVertexArray(const CVertexArray& va) = delete;
	CVertexArray(CVertexArray&& va) { *this = std::move(va); }

	virtual ~CVertexArray();

	CVertexArray& operator = (const CVertexArray& va) = delete;
	CVertexArray& operator = (CVertexArray&& va) {
		drawArray     = va.drawArray;     va.drawArray     = nullptr;
		drawArrayPos  = va.drawArrayPos;  va.drawArrayPos  = nullptr;
		drawArraySize = va.drawArraySize; va.drawArraySize = nullptr;

		stripArray     = va.stripArray;     va.stripArray     = nullptr;
		stripArrayPos  = va.stripArrayPos;  va.stripArrayPos  = nullptr;
		stripArraySize = va.stripArraySize; va.stripArraySize = nullptr;

		maxVertices = va.maxVertices;
		return *this;
	}

	bool IsReady() const;
	void Initialize();
	void CheckInitSize(const unsigned int vertexes, const unsigned int strips = 0);
	void EnlargeArrays(const unsigned int vertexes, const unsigned int strips = 0, const unsigned int stripsize = VA_SIZE_0);
	unsigned int drawIndex() const { return drawArrayPos - drawArray; }
	void ResetPos() { drawArrayPos = drawArray; }

	// standard API
	void AddVertex0(const float3& p);
	void AddVertex0(float x, float y, float z);
	void AddVertexN(const float3& p, const float3& n);
	void AddVertexT(const float3& p, float tx, float ty);
	void AddVertexC(const float3& p, const unsigned char* c);
	void AddVertexTC(const float3& p, float tx, float ty, const unsigned char* c);
	void AddVertexTN(const float3& p, float tx, float ty, const float3& n);
	void AddVertexTNT(const float3& p, float tx, float ty, const float3& n, const float3& st, const float3& tt);
	void AddVertex2d0(float x, float z);
	void AddVertex2dT(float x, float y, float tx, float ty);
	void AddVertex2dT(const float2 p, const float2 tc) { AddVertex2dT(p.x,p.y, tc.x,tc.y); }
	void AddVertex2dTC(float x, float y, float tx, float ty, const unsigned char* c);

	// same as the AddVertex... functions just without automated CheckEnlargeDrawArray
	void AddVertexQ0(float x, float y, float z);
	void AddVertexQ0(const float3& f3);
	void AddVertexQN(const float3& p, const float3& n);
	void AddVertexQC(const float3& p, const unsigned char* c);
	void AddVertexQT(const float3& p, float tx, float ty);
	void AddVertexQTN(const float3& p, float tx, float ty, const float3& n);
	void AddVertexQTNT(const float3& p, float tx, float ty, const float3& n, const float3& st, const float3& tt);
	void AddVertexQTC(const float3& p, float tx, float ty, const unsigned char* c);
	void AddVertexQ2d0(float x, float z);
	void AddVertexQ2dT(float x, float y, float tx, float ty);
	void AddVertexQ2dT(const float2 p, const float2 tc) { AddVertexQ2dT(p.x,p.y, tc.x,tc.y); }
	void AddVertexQ2dTC(float x, float y, float tx, float ty, const unsigned char* c);

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
	void CheckEnlargeDrawArray(size_t bytesNeeded);
	void EnlargeStripArray();
	void EnlargeDrawArray();
	void CheckEndStrip();

protected:
	float* drawArray;
	float* drawArrayPos;
	float* drawArraySize;

	unsigned int* stripArray;
	unsigned int* stripArrayPos;
	unsigned int* stripArraySize;

	unsigned int maxVertices;
};

#endif /* VERTEXARRAY_H */
