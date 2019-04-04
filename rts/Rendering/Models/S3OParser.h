/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef S3O_PARSER_H
#define S3O_PARSER_H

#include "3DModel.h"
#include "IModelParser.h"

#include "System/type2.h"

enum {
	S3O_PRIMTYPE_TRIANGLES      = 0,
	S3O_PRIMTYPE_TRIANGLE_STRIP = 1,
	S3O_PRIMTYPE_QUADS          = 2,
};


typedef SVertexData SS3OVertex;


struct SS3OPiece: public S3DModelPiece {
public:
	SS3OPiece() = default;
	SS3OPiece(const SS3OPiece&) = delete;
	SS3OPiece(SS3OPiece&& p) { *this = std::move(p); }

	SS3OPiece& operator = (const SS3OPiece& p) = delete;
	SS3OPiece& operator = (SS3OPiece&& p) {
		#if 0
		// piece is never actually moved, just need the operator for pool
		vertices = std::move(p.vertices);
		indices = std::move(p.indices);

		primType = p.primType;
		#endif
		return *this;
	}

	void Clear() override {
		S3DModelPiece::Clear();

		vertices.clear();
		indices.clear();

		primType = S3O_PRIMTYPE_TRIANGLES;
	}

	unsigned int GetVertexDrawIndexCount() const override { return indices.size(); }
	unsigned int GetVertexCount() const override { return vertices.size(); }
	const float3& GetVertexPos(const int idx) const override { return vertices[idx].pos; }
	const float3& GetNormal(const int idx) const override { return vertices[idx].normal; }

	const std::vector<SS3OVertex>& GetVertexElements() const override { return vertices; }
	const std::vector<unsigned>& GetVertexIndices() const override { return indices; }

public:
	void SetVertexCount(unsigned int n) { vertices.resize(n); }
	void SetIndexCount(unsigned int n) { indices.resize(n); }
	void SetVertex(int idx, const SS3OVertex& v) { vertices[idx] = v; }
	void SetIndex(int idx, const unsigned int drawIdx) { indices[idx] = drawIdx; }

	void Trianglize();
	void SetMinMaxExtends();
	void SetVertexTangents();

public:
	std::vector<SS3OVertex> vertices;
	std::vector<unsigned int> indices;

	int primType = S3O_PRIMTYPE_TRIANGLES;
};



class CS3OParser: public IModelParser
{
public:
	void Init() override;
	void Kill() override;

	S3DModel Load(const std::string& name) override;

private:
	SS3OPiece* AllocPiece();
	SS3OPiece* LoadPiece(S3DModel*, SS3OPiece*, std::vector<uint8_t>& buf, int offset);

private:
	std::vector<SS3OPiece> piecePool;
	spring::mutex poolMutex;

	unsigned int numPoolPieces = 0;
};

#endif /* S3O_PARSER_H */
