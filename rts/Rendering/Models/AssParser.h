/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ASS_PARSER_H
#define ASS_PARSER_H

#include "3DModel.h"
#include "IModelParser.h"

#include "System/float3.h"
#include "System/type2.h"

#include <vector>
#include <map>

#define NUM_MODEL_TEXTURES 2
#define NUM_MODEL_UVCHANNS 2

struct aiNode;
struct aiScene;
class LuaTable;

struct SAssVertex {
	SAssVertex() : normal(UpVector) {}

	float3 pos;
	float3 normal;
	float3 sTangent;
	float3 tTangent;

	//< Second channel is optional, still good to have. Also makes
	//< sure the struct is 64bytes in size (ATi's prefers such VBOs)
	//< supporting an arbitrary number of channels would be easy but
	//< overkill (for now)
	float2 texCoords[NUM_MODEL_UVCHANNS];
};

struct SAssPiece: public S3DModelPiece
{
	SAssPiece(): numTexCoorChannels(0) {
	}

	void DrawForList() const;
	void UploadGeometryVBOs();
	const float3& GetVertexPos(const int idx) const { return vertices[idx].pos; }
	const float3& GetNormal(const int idx) const { return vertices[idx].normal; }

	unsigned int GetVertexCount() const { return vertices.size(); }
	unsigned int GetNormalCount() const { return vertices.size(); }
	unsigned int GetTxCoorCount() const { return vertices.size(); }

	// FIXME implement
	// void Shatter(float, int, int, const float3&, const float3&) const

	unsigned int GetNumTexCoorChannels() const { return numTexCoorChannels; }
	void SetNumTexCoorChannels(unsigned int n) { numTexCoorChannels = n; }

public:
	std::vector<SAssVertex> vertices;
	std::vector<unsigned int> vertexDrawIndices;

	unsigned int numTexCoorChannels;
};


class CAssParser: public IModelParser
{
public:
	typedef std::map<std::string, S3DModelPiece*> ModelPieceMap;
	typedef std::map<SAssPiece*, std::string> ParentNameMap;

	CAssParser();
	S3DModel* Load(const std::string& modelFileName);
private:

	GLint maxIndices;
	GLint maxVertices;

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
		const aiNode* pieceNode,
		const aiScene* scene
	);
	static SAssPiece* LoadPiece(
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
	static bool SetModelRadiusAndHeight(
		S3DModel* model,
		const SAssPiece* piece,
		const aiNode* pieceNode,
		const LuaTable& pieceTable
	);
};

#endif /* ASS_PARSER_H */
