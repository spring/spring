/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef VERTEXARRAYRANGE_H
#define VERTEXARRAYRANGE_H

#include "VertexArray.h"
#include "myGL.h"

class CVertexArrayRange : public CVertexArray  
{
public:
	CVertexArrayRange(float* mem,int size);
	virtual ~CVertexArrayRange();

	virtual void Initialize();

	virtual void DrawArrayTC(int drawType,int stride=24);
	virtual void DrawArrayTN(int drawType,int stride=32);
	virtual void DrawArrayT2(int drawType,int stride=28);
	virtual void DrawArrayT(int drawType,int stride=20);

	virtual void EnlargeDrawArray();
	virtual bool IsReady() const;

  GLuint fence;
};

#endif /* VERTEXARRAYRANGE_H */
