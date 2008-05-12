#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H
// VertexArray.h: interface for the CVertexArray class.
//
//////////////////////////////////////////////////////////////////////

#include "myGL.h"

class CVertexArray
{
public:
	CVertexArray();
	~CVertexArray();
	void Initialize();
	bool IsReady();

	inline void AddVertexTC(const float3 &pos,float tx,float ty,unsigned char* color);
	void DrawArrayTC(int drawType,int stride=24);
	inline void AddVertexTN(const float3 &pos,float tx,float ty,const float3& norm);
	void DrawArrayTN(int drawType,int stride=32);
	inline void AddVertexT2(const float3& pos,float t1x,float t1y,float t2x,float t2y);
	void DrawArrayT2(int drawType,int stride=28);
	inline void AddVertexT(const float3& pos,float tx,float ty);
	void DrawArrayT(int drawType,int stride=20);
	inline void AddVertex0(const float3& pos);
	void DrawArray0(int drawType,int stride=12);
	inline void AddVertexC(const float3& pos,unsigned char* color);
	void DrawArrayC(int drawType,int stride=16);

	void EnlargeStripArray();
	void EnlargeDrawArray();
	void EndStrip();

	float* drawArray;
	int drawArraySize;
	int drawIndex;

	int* stripArray;
	int stripArraySize;
	int stripIndex;
};

/***************************************************************************************************/
/***************************************************************************************************/

inline void CVertexArray::AddVertexTC(const float3 &pos,float tx,float ty,unsigned char* color)
{
	if(drawIndex>drawArraySize-10)
		EnlargeDrawArray();

	drawArray[drawIndex++]=pos.x;
	drawArray[drawIndex++]=pos.y;
	drawArray[drawIndex++]=pos.z;
	drawArray[drawIndex++]=tx;
	drawArray[drawIndex++]=ty;
	drawArray[drawIndex++]=*((float*)(color));
}

inline void CVertexArray::AddVertexTN(const float3 &pos,float tx,float ty,const float3& norm)
{
	if(drawIndex>drawArraySize-10)
		EnlargeDrawArray();

	drawArray[drawIndex++]=pos.x;
	drawArray[drawIndex++]=pos.y;
	drawArray[drawIndex++]=pos.z;
	drawArray[drawIndex++]=tx;
	drawArray[drawIndex++]=ty;
	drawArray[drawIndex++]=norm.x;
	drawArray[drawIndex++]=norm.y;
	drawArray[drawIndex++]=norm.z;
}

inline void CVertexArray::AddVertexT2(const float3& pos,float t1x,float t1y,float t2x,float t2y)
{
	if(drawIndex>drawArraySize-10)
		EnlargeDrawArray();

	drawArray[drawIndex++]=pos.x;
	drawArray[drawIndex++]=pos.y;
	drawArray[drawIndex++]=pos.z;
	drawArray[drawIndex++]=t1x;
	drawArray[drawIndex++]=t1y;
	drawArray[drawIndex++]=t2x;
	drawArray[drawIndex++]=t2y;
}

inline void CVertexArray::AddVertexT(const float3& pos,float tx,float ty)
{
	if(drawIndex>drawArraySize-10)
		EnlargeDrawArray();

	drawArray[drawIndex++]=pos.x;
	drawArray[drawIndex++]=pos.y;
	drawArray[drawIndex++]=pos.z;
	drawArray[drawIndex++]=tx;
	drawArray[drawIndex++]=ty;
}

inline void CVertexArray::AddVertex0(const float3& pos)
{
	if(drawIndex>drawArraySize-10)
		EnlargeDrawArray();

	drawArray[drawIndex++]=pos.x;
	drawArray[drawIndex++]=pos.y;
	drawArray[drawIndex++]=pos.z;
}

inline void CVertexArray::AddVertexC(const float3& pos,unsigned char* color)
{
	if(drawIndex>drawArraySize-10)
		EnlargeDrawArray();

	drawArray[drawIndex++]=pos.x;
	drawArray[drawIndex++]=pos.y;
	drawArray[drawIndex++]=pos.z;
	drawArray[drawIndex++]=*((float*)(color));
}


#endif /* VERTEXARRAY_H */
