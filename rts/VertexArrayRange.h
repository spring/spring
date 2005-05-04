// VertexArrayRange.h: interface for the CVertexArrayRange class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_VERTEXARRAYRANGE_H__F600894E_153A_4A05_9C29_7650EC565F55__INCLUDED_)
#define AFX_VERTEXARRAYRANGE_H__F600894E_153A_4A05_9C29_7650EC565F55__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "VertexArray.h"
#include "mygl.h"

class CVertexArrayRange : public CVertexArray  
{
public:
	CVertexArrayRange(float* mem,int size);
	virtual ~CVertexArrayRange();

	virtual Initialize();

	virtual void DrawArrayTC(int drawType,int stride=24);
	virtual void DrawArrayTN(int drawType,int stride=32);
	virtual void DrawArrayT2(int drawType,int stride=28);
	virtual void DrawArrayT(int drawType,int stride=20);

	virtual void EnlargeDrawArray();
	virtual bool IsReady();

  GLuint fence;
};

#endif // !defined(AFX_VERTEXARRAYRANGE_H__F600894E_153A_4A05_9C29_7650EC565F55__INCLUDED_)

