/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ASS_PARSER_H
#define ASS_PARSER_H

#include <vector>

#include "3DModel.h"
#include "IModelParser.h"
#include "System/float3.h"
#include "System/type2.h"
#include "System/UnorderedMap.hpp"


struct aiNode;
struct aiScene;
class LuaTable;

typedef SVertexData SAssVertex;


struct SAssPiece: public S3DModelPiece
{
	SAssPiece() = default;
	SAssPiece(const SAssPiece&) = delete;
	SAssPiece(SAssPiece&& p) { *this = std::move(p); }

	SAssPiece& operator = (const SAssPiece& p) = delete;
	SAssPiece& operator = (SAssPiece&& p) {
		#if 0
		// piece is never actually moved, just need the operator for pool
		vertices = std::move(p.vertices);
		indices = std::move(p.indices);
		#endif
		return *this;
	}

	void Clear() override {
		S3DModelPiece::Clear();

		vertices.clear();
		indices.clear();

		numTexCoorChannels = 0;
	}

	unsigned int GetVertexCount() const override { return vertices.size(); }
	unsigned int GetVertexDrawIndexCount() const override { return indices.size(); }

	const float3& GetVertexPos(const int idx) const override { return vertices[idx].pos; }
	const float3& GetNormal(const int idx) const override { return vertices[idx].normal; }

	const std::vector<SAssVertex>& GetVertexElements() const override { return vertices; }
	const std::vector<unsigned>& GetVertexIndices() const override { return indices; }

	unsigned int GetNumTexCoorChannels() const { return numTexCoorChannels; }
	void SetNumTexCoorChannels(unsigned int n) { numTexCoorChannels = n; }

public:
	std::vector<SAssVertex> vertices;
	std::vector<unsigned int> indices;

	unsigned int numTexCoorChannels = 0;
};


class CAssParser: public IModelParser
{
public:
	typedef spring::unordered_map<std::string, S3DModelPiece*> ModelPieceMap;
	typedef spring::unordered_map<std::string, std::string> ParentNameMap;

	void Init() override;
	void Kill() override;

	S3DModel Load(const std::string& modelFileName) override;

private:
	static void PreProcessFileBuffer(std::vector<unsigned char>& fileBuffer);

	static void SetPieceName(
		SAssPiece* piece,
		const S3DModel* model,
		const aiNode* pieceNode,
		ModelPieceMap& pieceMap
	);
	static void SetPieceParentName(
		SAssPiece* piece,
		const S3DModel* model,
		const aiNode* pieceNode,
		const LuaTable& pieceTable,
		ParentNameMap& parentMap
	);
	static void LoadPieceTransformations(
		SAssPiece* piece,
		const S3DModel* model,
		const aiNode* pieceNode,
		const LuaTable& pieceTable
	);
	static void LoadPieceGeometry(
		SAssPiece* piece,
		const S3DModel* model,
		const aiNode* pieceNode,
		const aiScene* scene
	);

	SAssPiece* AllocPiece();
	SAssPiece* LoadPiece(
		S3DModel* model,
		const aiNode* pieceNode,
		const aiScene* scene,
		const LuaTable& modelTable,
		ModelPieceMap& pieceMap,
		ParentNameMap& parentMap
	);

	static void BuildPieceHierarchy(S3DModel* model, ModelPieceMap& pieceMap, const ParentNameMap& parentMap);
	static void CalculateModelDimensions(S3DModel* model, S3DModelPiece* piece);
	static void CalculateModelProperties(S3DModel* model, const LuaTable& pieceTable);
	static void FindTextures(
		S3DModel* model,
		const aiScene* scene,
		const LuaTable& pieceTable,
		const std::string& modelPath,
		const std::string& modelName
	);

private:
	unsigned int maxIndices = 0;
	unsigned int maxVertices = 0;
	unsigned int numPoolPieces = 0;

	std::vector<SAssPiece> piecePool;
	spring::mutex poolMutex;
};

#endif /* ASS_PARSER_H */
