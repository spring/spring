#ifndef ASSPARSER_H
#define ASSPARSER_H

#include <vector>
#include <map>
#include "IModelParser.h"
#include "float3.h"

struct aiNode;
struct aiScene;
class LuaTable;

struct SAssVertex {
	SAssVertex() : normal(UpVector), textureX(0.f), textureY(0.f), hasNormal(false), hasTangent(false) {}

	float3 pos;
	float3 normal;
	float textureX;
	float textureY;
	bool hasNormal;
	bool hasTangent;
};

struct SAssPiece: public S3DModelPiece
{
	SAssPiece() : node(NULL) {}

	void DrawForList() const;
	const float3& GetVertexPos(int idx) const { return vertices[idx].pos; }	
	
	//FIXME implement
	//int GetVertexCount() const { return 0; }
	//int GetNormalCount() const { return 0; }
	//int GetTxCoorCount() const { return 0; }
	//void SetMinMaxExtends() {}
	//void SetVertexTangents() {}
	//float3 GetPosOffset() const { return ZeroVector; }
	//void Shatter(float, int, int, const float3&, const float3&) const
	
public:
	aiNode* node;
	std::vector<SAssVertex> vertices;
	std::vector<unsigned int> vertexDrawOrder;

	//! cannot store these in SAssVertex
	std::vector<float3> sTangents; //! == T(angent) dirs
	std::vector<float3> tTangents; //! == B(itangent) dirs
};

struct SAssModel: public S3DModel
{
	SAssModel() : scene(NULL) {}
	
public:
	struct MinMax {
		float3 mins,maxs;
	};
	std::vector<MinMax> mesh_minmax;

	const aiScene* scene;       //! Assimp scene containing all loaded model data. NULL for S30/3DO.
};


class CAssParser: public IModelParser
{
public:
	S3DModel* Load(const std::string& modelFileName);

private:
	static SAssPiece* LoadPiece(SAssModel* model, aiNode* node, const LuaTable& metaTable);
	static void BuildPieceHierarchy(S3DModel* model);
	static void CalculateRadius(S3DModel* model);
	static void CalculateHeight(S3DModel* model);
	static void CalculateMinMax(S3DModelPiece* piece);
	static void LoadPieceTransformations(SAssPiece* piece, const LuaTable& metaTable);
	
	void CalculatePerMeshMinMax(SAssModel* model);
};

#endif /* ASSPARSER_H */
