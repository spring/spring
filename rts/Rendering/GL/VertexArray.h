#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H
// VertexArray.h: interface for the CVertexArray class.
//
//////////////////////////////////////////////////////////////////////

class CVertexArray  
{
public:
	CVertexArray();
	~CVertexArray();
	void Initialize();

	void AddVertexTC(const float3 &pos,float tx,float ty,unsigned char* color);
	void DrawArrayTC(int drawType,int stride=24);
	void AddVertexTN(const float3 &pos,float tx,float ty,const float3& norm);
	void DrawArrayTN(int drawType,int stride=32);
	void AddVertexT2(const float3& pos,float t1x,float t1y,float t2x,float t2y);
	void DrawArrayT2(int drawType,int stride=28);
	void AddVertexT(const float3& pos,float tx,float ty);
	void DrawArrayT(int drawType,int stride=20);
	void AddVertex0(const float3& pos);
	void DrawArray0(int drawType,int stride=12);
	void AddVertexC(const float3& pos,unsigned char* color);
	void DrawArrayC(int drawType,int stride=16);

	void EnlargeStripArray();
	void EnlargeDrawArray();
	void EndStrip();
	bool IsReady();

	float* drawArray;
	int drawArraySize;
	int drawIndex;

	int* stripArray;
	int stripArraySize;
	int stripIndex;
};

#endif /* VERTEXARRAY_H */
