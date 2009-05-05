#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H
#include "myGL.h"
#include "Platform/errorhandler.h"
// VertexArray.h: interface for the CVertexArray class.
//
//////////////////////////////////////////////////////////////////////

#define VA_INIT_VERTEXES 1000 // please don't change this, some files rely on specific initial sizes
#define VA_INIT_STRIPS 100
#define VA_SIZE_0 3
#define VA_SIZE_C 4
#define VA_SIZE_T 5
#define VA_SIZE_TN 8
#define VA_SIZE_TC 6
#define VA_SIZE_2DT 4

class CVertexArray  
{
public:
	typedef void (*StripCallback)(void* data);

public:
	CVertexArray();
	~CVertexArray();
	void Initialize();
	inline void CheckInitSize(const unsigned int vertexes, const unsigned int strips=0);

	inline void AddVertex0(const float3& pos);
	inline void AddVertex0(const float x, const float y, const float z);
	inline void AddVertexT(const float3& pos,const float tx,const float ty);
	inline void AddVertexC(const float3& pos,const unsigned char* color);
	inline void AddVertexTC(const float3 &pos,const float tx,const float ty,const unsigned char* color);
	inline void AddVertexTN(const float3 &pos,const float tx,const float ty,const float3& norm);
	inline void AddVertexT2(const float3& pos,const float t1x,const float t1y,const float t2x,const float t2y);
	inline void AddVertex2dT(const float x,const float y,const float tx,const float ty);

	void DrawArray0(const int drawType, unsigned int stride=12);
	void DrawArrayT(const int drawType, unsigned int stride=20);
	void DrawArrayC(const int drawType, unsigned int stride=16);
	void DrawArrayTC(const int drawType, unsigned int stride=24);
	void DrawArrayTN(const int drawType, unsigned int stride=32);
	void DrawArrayT2(const int drawType, unsigned int stride=28);
	void DrawArray2dT(const int drawType, unsigned int stride=16);
	void DrawArray2dT(const int drawType, StripCallback callback, void* data, unsigned int stride=16);

	//! same as the AddVertex... functions just without automated CheckEnlargeDrawArray
	inline void AddVertexQ0(const float x, const float y, const float z);
	inline void AddVertexQC(const float3& pos,const unsigned char* color);
	inline void AddVertexQT(const float3& pos,const float tx,const float ty);
	inline void AddVertex2dQT(const float x,const float y,const float tx,const float ty);
	inline void AddVertexQTN(const float3 &pos,const float tx,const float ty,const float3& norm);
	inline void AddVertexQTC(const float3 &pos,const float tx,const float ty,const unsigned char* color);

	//! same as EndStrip, but without automated EnlargeStripArray
	inline void EndStripQ();

	bool IsReady();
	inline unsigned int drawIndex() const;
	void EndStrip();
	inline void EnlargeArrays(const unsigned int vertexes, const unsigned int strips, const unsigned int stripsize=VA_SIZE_0);

	float* drawArray;
	float* drawArrayPos;
	float* drawArraySize;

	unsigned int* stripArray;
	unsigned int* stripArrayPos;
	unsigned int* stripArraySize;

	static unsigned int maxVertices;

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
