#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H
#include "myGL.h"
// VertexArray.h: interface for the CVertexArray class.
//
//////////////////////////////////////////////////////////////////////

class CVertexArray  
{
public:
	CVertexArray();
	~CVertexArray();
	void Initialize();

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

	//! same as AddVertex0, but without autmated CheckEnlargeDrawArray
	inline void AddVertexQ0(float x, float y, float z);

	//! same as EndStrip, but without autmated EnlargeStripArray
	inline void EndStripQ();

	bool IsReady();
	inline int drawIndex();
	void EndStrip();
	void EnlargeArrays(int vertexes, int strips, int stripsize=3);

	float* drawArray;
	float* drawArrayPos;
	float* drawArraySize;

	int* stripArray;
	int* stripArrayPos;
	int* stripArraySize;

protected:
	inline void DrawArrays(int drawType, int stride);
	inline void CheckEnlargeDrawArray();
	void EnlargeStripArray();
	void EnlargeDrawArray();
	void CheckEndStrip();
};

inline void CVertexArray::DrawArrays(int drawType, int stride) {
	int newIndex,oldIndex=0;
	int *stripArrayPtr=stripArray;
	while(stripArrayPtr<stripArrayPos) {
		newIndex=(*stripArrayPtr++)/stride;
		glDrawArrays(drawType,oldIndex,newIndex-oldIndex);
		oldIndex=newIndex;
	}
}

inline void CVertexArray::CheckEnlargeDrawArray() {
	if((char *)drawArrayPos>(char *)drawArraySize-10*sizeof(float))
		EnlargeDrawArray();
}

inline void CVertexArray::AddVertexQ0(float x, float y, float z) {
	*drawArrayPos++=x;
	*drawArrayPos++=y;
	*drawArrayPos++=z;
}

inline void CVertexArray::AddVertex0(const float3& pos) {
	if(drawArrayPos>drawArraySize-10)
		EnlargeDrawArray();

	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
}

inline void CVertexArray::AddVertexC(const float3& pos,unsigned char* color) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=*((float*)(color));
}

inline void CVertexArray::AddVertexT(const float3& pos,float tx,float ty) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
}

inline void CVertexArray::AddVertexT2(const float3& pos,float tx,float ty,float t2x,float t2y) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
	*drawArrayPos++=t2x;
	*drawArrayPos++=t2y;
}

inline void CVertexArray::AddVertexTN(const float3& pos,float tx,float ty,const float3& norm) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
	*drawArrayPos++=norm.x;
	*drawArrayPos++=norm.y;
	*drawArrayPos++=norm.z;
}

inline void CVertexArray::AddVertexTC(const float3& pos,float tx,float ty,unsigned char* col) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
	*drawArrayPos++=*((float*)(col));
}


inline void CVertexArray::CheckEndStrip() {
	if(stripArrayPos==stripArray || *(stripArrayPos-1)!=((char *)drawArrayPos-(char *)drawArray))
		EndStrip();
}

inline int CVertexArray::drawIndex() {
	return drawArrayPos-drawArray;
}

inline void CVertexArray::EndStripQ() {
	*stripArrayPos++=((char *)drawArrayPos-(char *)drawArray);
}

#endif /* VERTEXARRAY_H */
