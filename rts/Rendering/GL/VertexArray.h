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

class CVertexArray  
{
public:
	CVertexArray();
	~CVertexArray();
	void Initialize();
	inline void CheckInitSize(const int vertexes, const int strips=0);

	inline void AddVertexTC(const float3 &pos,const float tx,const float ty,const unsigned char* color);
	void DrawArrayTC(const int drawType,int stride=24);
	inline void AddVertexTN(const float3 &pos,const float tx,const float ty,const float3& norm);
	void DrawArrayTN(const int drawType,int stride=32);
	inline void AddVertexT2(const float3& pos,const float t1x,const float t1y,const float t2x,const float t2y);
	void DrawArrayT2(const int drawType,int stride=28);
	inline void AddVertexT(const float3& pos,const float tx,const float ty);
	void DrawArrayT(const int drawType,int stride=20);
	inline void AddVertex0(const float3& pos);
	inline void AddVertex0(const float x, const float y, const float z);
	void DrawArray0(const int drawType,int stride=12);
	inline void AddVertexC(const float3& pos,const unsigned char* color);
	void DrawArrayC(const int drawType,int stride=16);

	//! same as AddVertex0, but without automated CheckEnlargeDrawArray
	inline void AddVertexQ0(const float x, const float y, const float z);
	//! same as AddVertexC, but without automated CheckEnlargeDrawArray
	inline void AddVertexQC(const float3& pos,const unsigned char* color);
	//! same as AddVertexT, but without automated CheckEnlargeDrawArray
	inline void AddVertexQT(const float3& pos,const float tx,const float ty);
	//! same as AddVertexTN, but without automated CheckEnlargeDrawArray
	inline void AddVertexQTN(const float3 &pos,const float tx,const float ty,const float3& norm);
	//! same as AddVertexTC, but without automated CheckEnlargeDrawArray
	inline void AddVertexQTC(const float3 &pos,const float tx,const float ty,const unsigned char* color);

	//! same as EndStrip, but without automated EnlargeStripArray
	inline void EndStripQ();

	bool IsReady();
	inline int drawIndex() const;
	void EndStrip();
	inline void EnlargeArrays(const int vertexes, const int strips, const int stripsize=VA_SIZE_0);

	float* drawArray;
	float* drawArrayPos;
	float* drawArraySize;

	int* stripArray;
	int* stripArrayPos;
	int* stripArraySize;

protected:
	inline void DrawArrays(const int drawType, const int stride);
	inline void CheckEnlargeDrawArray();
	void EnlargeStripArray();
	void EnlargeDrawArray();
	void CheckEndStrip();
};

inline void CVertexArray::DrawArrays(const int drawType, const int stride) {
	int newIndex,oldIndex=0;
	int *stripArrayPtr=stripArray;
	while(stripArrayPtr<stripArrayPos) {
		newIndex=(*stripArrayPtr++)/stride;
		glDrawArrays(drawType,oldIndex,newIndex-oldIndex);
		oldIndex=newIndex;
	}
}

// calls to this function will be removed by the optimizer unless the size is too small
inline void CVertexArray::CheckInitSize(const int vertexes, const int strips) {
	if(vertexes>VA_INIT_VERTEXES || strips>VA_INIT_STRIPS) { 
		handleerror(drawArrayPos=NULL, "Vertex array initial size is too small", "Rendering error", MBF_OK | MBF_EXCL);
	}
}

inline void CVertexArray::CheckEnlargeDrawArray() {
	if((char *)drawArrayPos>(char *)drawArraySize-10*sizeof(float))
		EnlargeDrawArray();
}

inline void CVertexArray::EnlargeArrays(const int vertexes, const int strips, const int stripsize) {
	while((char *)drawArrayPos>(char *)drawArraySize-stripsize*sizeof(float)*vertexes)
		EnlargeDrawArray();

	while((char *)stripArrayPos>(char *)stripArraySize-sizeof(int)*strips)
		EnlargeStripArray();
}

inline void CVertexArray::AddVertexQ0(const float x, const float y, const float z) {
	assert(drawArraySize>=drawArrayPos+VA_SIZE_0);
	*drawArrayPos++=x;
	*drawArrayPos++=y;
	*drawArrayPos++=z;
}

inline void CVertexArray::AddVertexQC(const float3& pos,const unsigned char* color) {
	assert(drawArraySize>=drawArrayPos+VA_SIZE_C);
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=*((float*)(color));
}

inline void CVertexArray::AddVertexQT(const float3& pos,const float tx,const float ty) {
	assert(drawArraySize>=drawArrayPos+VA_SIZE_T);
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
}

inline void CVertexArray::AddVertexQTN(const float3& pos,const float tx,const float ty,const float3& norm) {
	assert(drawArraySize>=drawArrayPos+VA_SIZE_TN);
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
	*drawArrayPos++=norm.x;
	*drawArrayPos++=norm.y;
	*drawArrayPos++=norm.z;
}

inline void CVertexArray::AddVertexQTC(const float3& pos,const float tx,const float ty,const unsigned char* col) {
	assert(drawArraySize>=drawArrayPos+VA_SIZE_TC);
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
	*drawArrayPos++=*((float*)(col));
}

inline void CVertexArray::AddVertex0(const float3& pos) {
	if(drawArrayPos>drawArraySize-10)
		EnlargeDrawArray();

	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
}

inline void CVertexArray::AddVertex0(const float x, const float y, const float z) {
	if(drawArrayPos>drawArraySize-10)
		EnlargeDrawArray();

	*drawArrayPos++=x;
	*drawArrayPos++=y;
	*drawArrayPos++=z;
}

inline void CVertexArray::AddVertexC(const float3& pos,const unsigned char* color) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=*((float*)(color));
}

inline void CVertexArray::AddVertexT(const float3& pos,const float tx,const float ty) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
}

inline void CVertexArray::AddVertexT2(const float3& pos,const float tx,const float ty,const float t2x,const float t2y) {
	CheckEnlargeDrawArray();
	*drawArrayPos++=pos.x;
	*drawArrayPos++=pos.y;
	*drawArrayPos++=pos.z;
	*drawArrayPos++=tx;
	*drawArrayPos++=ty;
	*drawArrayPos++=t2x;
	*drawArrayPos++=t2y;
}

inline void CVertexArray::AddVertexTN(const float3& pos,const float tx,const float ty,const float3& norm) {
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

inline void CVertexArray::AddVertexTC(const float3& pos,const float tx,const float ty,const unsigned char* col) {
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

inline int CVertexArray::drawIndex() const {
	return drawArrayPos-drawArray;
}

inline void CVertexArray::EndStripQ() {
	assert(stripArraySize>=stripArrayPos+1);
	*stripArrayPos++=((char *)drawArrayPos-(char *)drawArray);
}

#endif /* VERTEXARRAY_H */
