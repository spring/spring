/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _ADV_TREE_GENERATOR_H
#define _ADV_TREE_GENERATOR_H

#include "Rendering/GL/myGL.h"

// XXX This has a duplicate in ITreeDrawer.h
#define MAX_TREE_HEIGHT 60

class CVertexArray;

namespace Shader {
	struct IProgramObject;
}

class CAdvTreeGenerator  
{
public:
	CAdvTreeGenerator();
	virtual ~CAdvTreeGenerator();

	void Draw() const;

	GLuint barkTex;
	GLuint farTex[2];
	unsigned int leafDL;
	unsigned int pineDL;

	CVertexArray* va;
	CVertexArray* barkva;

	void MainTrunk(int numBranch, float height, float width);
	void CreateFarTex(Shader::IProgramObject*);
	void CreateFarView(unsigned char* mem, int dx, int dy, unsigned int displist);

private:
	void CreateLeaves(const float3& start, const float3& dir, float length, float3& orto1, float3& orto2);
	void TrunkIterator(const float3& start, const float3& dir, float length, float size, int depth);
	void DrawTrunk(const float3& start, const float3& end, const float3& orto1, const float3& orto2, float size);
	void DrawPineTrunk(const float3& start, const float3& end, float size);
	void DrawPineBranch(const float3& start, const float3& dir, float size);
	void CreateGranTexBranch(const float3& start, const float3& end);
	void CreateGranTex(unsigned char* data, int xpos, int ypos, int xsize);
	void PineTree(int numBranch, float height);
	void CreateLeafTex(unsigned int baseTex, int xpos, int ypos, unsigned char buf[256][2048][4]);
};

extern CAdvTreeGenerator* treeGen;

#endif // _ADV_TREE_GENERATOR_H

