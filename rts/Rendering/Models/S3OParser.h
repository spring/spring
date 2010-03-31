/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef S3OPARSER_H
#define S3OPARSER_H

#include <map>
#include "IModelParser.h"


struct SS3OVertex {
	float3 pos;
	float3 normal;
	float textureX;
	float textureY;
};

struct SS3OPiece: public S3DModelPiece {
	~SS3OPiece() {
		vertices.clear();
		vertexDrawOrder.clear();

		sTangents.clear();
		tTangents.clear();
	}

	void DrawList() const;
	void SetVertexTangents();
	const float3& GetVertexPos(int idx) const { return vertices[idx].pos; }
	void Shatter(float, int, int, const float3&, const float3&) const;

	std::vector<SS3OVertex> vertices;
	std::vector<unsigned int> vertexDrawOrder;
	int primitiveType;

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


enum {S3O_PRIMTYPE_TRIANGLES, S3O_PRIMTYPE_TRIANGLE_STRIP, S3O_PRIMTYPE_QUADS};

class CS3OParser: public IModelParser
{
public:
	S3DModel* Load(const std::string& name);

private:
	SS3OPiece* LoadPiece(unsigned char* buf, int offset, S3DModel* model);
	void FindMinMax(SS3OPiece* object) const;
	void SetVertexTangents(SS3OPiece*);
};

#endif /* S3OPARSER_H */
