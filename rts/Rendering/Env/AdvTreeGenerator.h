/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __ADV_TREE_GENERATOR_H__
#define __ADV_TREE_GENERATOR_H__

#include "Rendering/GL/myGL.h"
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
	void FixAlpha(unsigned char* data);
	void FixAlpha2(unsigned char* data);
	void CreateLeaves(const float3& start, const float3& dir, float length, float3& orto1, float3& orto2);
	void TrunkIterator(const float3& start, const float3& dir, float length, float size, int depth);
	void DrawTrunk(const float3& start, const float3& end, const float3& orto1, const float3& orto2, float size);
	void DrawPineTrunk(const float3& start, const float3& end, float size);
	void DrawPineBranch(const float3& start, const float3& dir, float size);
	void CreateGranTexBranch(const float3& start, const float3& end);
	void CreateTex(unsigned char* data, unsigned int tex, int xsize, int ysize, bool fixAlpha, int maxMipLevel);
	void CreateGranTex(unsigned char* data, int xpos, int ypos, int xsize);
	void PineTree(int numBranch, float height);
	float fRand(float size);
	void CreateLeafTex(unsigned int baseTex, int xpos, int ypos, unsigned char buf[256][2048][4]);
};

extern CAdvTreeGenerator* treeGen;

#endif // __ADV_TREE_GENERATOR_H__

