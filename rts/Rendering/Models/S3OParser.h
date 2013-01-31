/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef S3O_PARSER_H
#define S3O_PARSER_H

#include <map>
#include "IModelParser.h"


struct SS3OVertex {
	float3 pos;
	float3 normal;
	float textureX;
	float textureY;
};

struct SS3OPiece: public S3DModelPiece {
	SS3OPiece() { parent = NULL; primitiveType = 0; }

	void UploadGeometryVBOs();
	void DrawForList() const;

	void SetVertexCount(unsigned int n) { vertices.resize(n); }
	void SetVertexDrawIndexCount(unsigned int n) { vertexDrawIndices.resize(n); }

	void SetMinMaxExtends();
	void SetVertexTangents();

	void SetVertex(int idx, const SS3OVertex& v) { vertices[idx] = v; }
	void SetVertexDrawIndex(int idx, const unsigned int drawIdx) { vertexDrawIndices[idx] = drawIdx; }

	unsigned int GetVertexCount() const { return vertices.size(); }
	unsigned int GetVertexDrawIndexCount() const { return vertexDrawIndices.size(); }

	const float3& GetVertexPos(const int idx) const { return vertices[idx].pos; }
	const float3& GetNormal(const int idx) const { return vertices[idx].normal; }
	void Shatter(float pieceChance, int texType, int team, const float3& pos,
			const float3& speed) const;

	int primitiveType;

private:
	std::vector<SS3OVertex> vertices;
	std::vector<unsigned int> vertexDrawIndices;

	// cannot store these in SS3OVertex
	std::vector<float3> sTangents; // == T(angent) dirs
	std::vector<float3> tTangents; // == B(itangent) dirs
};

struct SS3OTriangle {
	unsigned int v0idx;
	unsigned int v1idx;
	unsigned int v2idx;
	float3 sTangent;
	float3 tTangent;
};


enum {S3O_PRIMTYPE_TRIANGLES = 0, S3O_PRIMTYPE_TRIANGLE_STRIP = 1, S3O_PRIMTYPE_QUADS = 2};

class CS3OParser: public IModelParser
{
public:
	S3DModel* Load(const std::string& name);

private:
	SS3OPiece* LoadPiece(S3DModel*, SS3OPiece*, unsigned char* buf, int offset);
};

#endif /* S3O_PARSER_H */
