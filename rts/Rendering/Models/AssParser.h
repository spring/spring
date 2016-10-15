/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ASS_PARSER_H
#define ASS_PARSER_H

#include <vector>
#include <map>
#include "3DModel.h"
#include "IModelParser.h"
#include "System/float3.h"
#include "System/type2.h"


struct aiNode;
struct aiScene;
class LuaTable;

typedef SVertexData SAssVertex;


struct SAssPiece: public S3DModelPiece
{
	SAssPiece(): numTexCoorChannels(0) {
	}

	void DrawForList() const override;
	void UploadGeometryVBOs() override;
	void BindVertexAttribVBOs() const override;
	void UnbindVertexAttribVBOs() const override;

	unsigned int GetVertexCount() const override { return vertices.size(); }
	unsigned int GetVertexDrawIndexCount() const override { return indices.size(); }
	const float3& GetVertexPos(const int idx) const override { return vertices[idx].pos; }
	const float3& GetNormal(const int idx) const override { return vertices[idx].normal; }
	const std::vector<unsigned>& GetVertexIndices() const override { return indices; }

	unsigned int GetNumTexCoorChannels() const { return numTexCoorChannels; }
	void SetNumTexCoorChannels(unsigned int n) { numTexCoorChannels = n; }

public:
	std::vector<SAssVertex> vertices;
	std::vector<unsigned int> indices;

	unsigned int numTexCoorChannels;
};


class CAssParser: public IModelParser
{
public:
	typedef std::map<std::string, S3DModelPiece*> ModelPieceMap;
	typedef std::map<SAssPiece*, std::string> ParentNameMap;

	CAssParser();
	~CAssParser();
	S3DModel* Load(const std::string& modelFileName);
	ModelType GetType() const { return MODELTYPE_ASS; }
private:
#ifndef BITMAP_NO_OPENGL
	GLint maxIndices;
	GLint maxVertices;
#endif

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
};

#endif /* ASS_PARSER_H */
