/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef S3O_PARSER_H
#define S3O_PARSER_H

#include "3DModel.h"
#include "IModelParser.h"

#include "System/type2.h"

#include <map>

enum {
	S3O_PRIMTYPE_TRIANGLES      = 0,
	S3O_PRIMTYPE_TRIANGLE_STRIP = 1,
	S3O_PRIMTYPE_QUADS          = 2,
};

struct SS3OVertex {
	float3 pos;
	float3 normal;
	float2 texCoord;
	float3 sTangent;
	float3 tTangent;
	char unused[8]; //Note: ATi wants 64 _byte_ aligned data in VBOs for optimal performance
};

struct SS3OPiece: public S3DModelPiece {
	SS3OPiece(): primType(S3O_PRIMTYPE_TRIANGLES) {
	}

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
	void Shatter(float pieceChance, int texType, int team, const float3& pos, const float3& speed) const;

	int primType;

private:
	std::vector<SS3OVertex> vertices;
	std::vector<unsigned int> vertexDrawIndices;
};



class CS3OParser: public IModelParser
{
public:
	S3DModel* Load(const std::string& name);

private:
	SS3OPiece* LoadPiece(S3DModel*, SS3OPiece*, unsigned char* buf, int offset);
};

#endif /* S3O_PARSER_H */
