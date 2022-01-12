/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_3DOPARSER_H
#define SPRING_3DOPARSER_H

#include <vector>
#include <string>

#include "3DModel.h"
#include "IModelParser.h"

#include "Rendering/Textures/3DOTextureHandler.h"
#include "System/float3.h"
#include "System/UnorderedSet.hpp"


typedef SVertexData S3DOVertex;

namespace TA3DO {
	typedef struct _3DObject
	{
		int VersionSignature;
		int NumberOfVertices;
		int NumberOfPrimitives;
		int SelectionPrimitive;
		int XFromParent;
		int YFromParent;
		int ZFromParent;
		int OffsetToObjectName;
		int Always_0;
		int OffsetToVertexArray;
		int OffsetToPrimitiveArray;
		int OffsetToSiblingObject;
		int OffsetToChildObject;
	} _3DObject;

	typedef struct _Primitive
	{
		int PaletteEntry;
		int NumberOfVertexIndexes;
		int Always_0;
		int OffsetToVertexIndexArray;
		int OffsetToTextureName;
		int Unknown_1;
		int Unknown_2;
		int Unknown_3;
	} _Primitive;
};

struct S3DOPrimitive
{
	std::vector<int>    indices;  ///< indices to S3DOPiece::verts
	std::vector<float3> vnormals; ///< per-vertex normals

	// the raw normal for this primitive (-v0v1.cross(v0v2))
	// used iff we have less than 3 or more than 4 vertices
	float3 primNormal;

	C3DOTextureHandler::UnitTexture* texture = nullptr;

	// which piece this primitive belongs to
	unsigned int pieceIndex = 0;
};


struct S3DOPiece: public S3DModelPiece
{
	S3DOPiece() = default;
	S3DOPiece(const S3DOPiece&) = delete;
	S3DOPiece(S3DOPiece&& p) { *this = std::move(p); }

	S3DOPiece& operator = (const S3DOPiece& p) = delete;
	S3DOPiece& operator = (S3DOPiece&& p) {
		#if 0
		// piece is never actually moved, just need the operator for pool
		emitPos = p.emitPos;
		emitDir = p.emitDir;

		verts = std::move(p.verts);
		prims = std::move(p.prims);

		vertexAttribs = std::move(p.vertexAttribs);
		vertexIndices = std::move(p.vertexIndices);
		#endif
		return *this;
	}

	void Clear() override {
		S3DModelPiece::Clear();

		verts.clear();
		prims.clear();

		vertexAttribs.clear();
		vertexIndices.clear();

		emitPos = ZeroVector;
		emitDir = ZeroVector;
	}

	unsigned int GetVertexCount() const override { return (vertexAttribs.size()); }
	unsigned int GetVertexDrawIndexCount() const override { return (vertexIndices.size()); }

	const float3& GetVertexPos(const int idx) const override { return vertexAttribs[idx].pos; }
	const float3& GetNormal(const int idx)    const override { return vertexAttribs[idx].normal; }

	const std::vector<S3DOVertex>& GetVertexElements() const override { return vertexAttribs; }
	const std::vector<unsigned>& GetVertexIndices() const override { return vertexIndices; }

	float3 GetEmitPos() const override { return emitPos; }
	float3 GetEmitDir() const override { return emitDir; }

public:
	void SetMinMaxExtends();
	void CalcNormals();
	void GenTriangleGeometry();

	void GetVertices(const TA3DO::_3DObject* o, const std::vector<unsigned char>& fileBuf);
	void GetPrimitives(
		const S3DModel* model,
		int pos,
		int num,
		int excludePrim,
		const std::vector<unsigned char>& fileBuf,
		const spring::unordered_set<std::string>& teamTextures
	);

	bool IsBasePlate(const S3DOPrimitive* face) const;

	C3DOTextureHandler::UnitTexture* GetTexture(
		const TA3DO::_Primitive* p,
		const std::vector<unsigned char>& fileBuf,
		const spring::unordered_set<std::string>& teamTextures
	) const;

public:
	std::vector<float3> verts; //FIXME
	std::vector<S3DOPrimitive> prims;

	std::vector<S3DOVertex> vertexAttribs;
	std::vector<unsigned int> vertexIndices;

	float3 emitPos;
	float3 emitDir;
};


class C3DOParser: public IModelParser
{
public:
	void Init() override;
	void Kill() override;

	S3DModel Load(const std::string& name) override;

	S3DOPiece* AllocPiece();
	S3DOPiece* LoadPiece(S3DModel* model, S3DOPiece* parent, const std::vector<uint8_t>& buf, int pos);

private:
	spring::unordered_set<std::string> teamTextures;
	std::vector<S3DOPiece> piecePool;
	spring::mutex poolMutex;

	unsigned int numPoolPieces = 0;
};

#endif // SPRING_3DOPARSER_H

