/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef ASS_PARSER_H
#define ASS_PARSER_H

#include <vector>
#include <map>
#include "IModelParser.h"
#include "System/float3.h"
#include "System/Vec2.h"

struct aiNode;
struct aiScene;
class LuaTable;

struct SAssVertex {
	SAssVertex() : normal(UpVector) {}

	float3 pos;
	float3 normal;
	float2 texCoord;
	float2 texCoord2; //< optional, still good to have. Also makes sure the struct is 64byte in size (ATi's prefers such VBOs)
	float3 sTangent;
	float3 tTangent;
};

struct SAssPiece: public S3DModelPiece
{
	SAssPiece() : node(NULL), hasTexCoord2(false) {}

	void DrawForList() const;
	void UploadGeometryVBOs();
	const float3& GetVertexPos(const int idx) const { return vertices[idx].pos; }	
	const float3& GetNormal(const int idx) const { return vertices[idx].normal; }
	
	unsigned int GetVertexCount() const { return vertices.size(); }
	unsigned int GetNormalCount() const { return vertices.size(); }
	unsigned int GetTxCoorCount() const { return vertices.size(); }

	//FIXME implement
	//void Shatter(float, int, int, const float3&, const float3&) const
	
public:
	aiNode* node;
	std::vector<SAssVertex> vertices;
	std::vector<unsigned int> vertexDrawIndices;

	bool hasTexCoord2;
};

struct SAssModel: public S3DModel
{
	SAssModel() : scene(NULL) {}
	
public:
	struct MinMax {
		float3 mins,maxs;
	};
	std::vector<MinMax> mesh_minmax;

	const aiScene* scene; //< Assimp scene containing all loaded model data. NULL for S30/3DO.
};


class CAssParser: public IModelParser
{
public:
	S3DModel* Load(const std::string& modelFileName);
	ModelType GetType() const { return MODELTYPE_ASS; }

private:
	static SAssPiece* LoadPiece(SAssModel* model, aiNode* node, const LuaTable& metaTable);
	static void BuildPieceHierarchy(S3DModel* model);
	static void CalculateModelDimensions(S3DModel* model, S3DModelPiece* piece);
	static void CalculateModelProperties(S3DModel* model, const LuaTable& metaTable);
	static void FindTextures(S3DModel* model, const aiScene* scene, const LuaTable& metaTable, const std::string& modelFilePath);
	static void LoadPieceTransformations(const S3DModel* model, SAssPiece* piece, const LuaTable& metaTable);
	
	void CalculatePerMeshMinMax(SAssModel* model);
};

#endif /* ASS_PARSER_H */
