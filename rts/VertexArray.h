#ifndef VERTEXARRAY_H
#define VERTEXARRAY_H
// VertexArray.h: interface for the CVertexArray class.
//
//////////////////////////////////////////////////////////////////////

class CVertexArray  
{
public:
	CVertexArray();
	virtual ~CVertexArray();
	virtual void Initialize();

	void AddVertexTC(const float3 &pos,float tx,float ty,unsigned char* color);
	virtual void DrawArrayTC(int drawType,int stride=24);
	void AddVertexTN(const float3 &pos,float tx,float ty,const float3& norm);
	virtual void DrawArrayTN(int drawType,int stride=32);
	void AddVertexT2(const float3& pos,float t1x,float t1y,float t2x,float t2y);
	virtual void DrawArrayT2(int drawType,int stride=28);
	void AddVertexT(const float3& pos,float tx,float ty);
	virtual void DrawArrayT(int drawType,int stride=20);
	void AddVertex0(const float3& pos);
	virtual void DrawArray0(int drawType,int stride=12);
	void AddVertexC(const float3& pos,unsigned char* color);
	virtual void DrawArrayC(int drawType,int stride=16);

	virtual void EnlargeStripArray();
	virtual void EnlargeDrawArray();
	void EndStrip();
	virtual bool IsReady();

	float* drawArray;
	int drawArraySize;
	int drawIndex;

	int* stripArray;
	int stripArraySize;
	int stripIndex;
};

#endif /* VERTEXARRAY_H */
