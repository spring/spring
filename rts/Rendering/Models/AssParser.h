#ifndef ASSPARSER_H
#define ASSPARSER_H

#include <map>
#include "IModelParser.h"
#include "float3.h"

struct aiNode;
struct aiScene;
class LuaTable;

struct SAssVertex {
	float3 pos;
	float3 normal;
	float textureX;
	float textureY;
	bool hasNormal;
	bool hasTangent;
};

struct SAssPiece: public S3DModelPiece
{
public:
	aiNode* node;

	const float3& GetVertexPos(int idx) const { return vertices[idx].pos; }
	void DrawList() const;
	std::vector<SAssVertex> vertices;

	std::vector<unsigned int> vertexDrawOrder;
	// cannot store these in SAssVertex
	std::vector<float3> sTangents; // == T(angent) dirs
	std::vector<float3> tTangents; // == B(itangent) dirs
};


class CAssParser: public IModelParser
{
public:
	S3DModel* Load(const std::string& modelFileName); ///< Load a model
	void Draw(const S3DModelPiece* o) const; ///< Build displaylist for loaded model

private:
	SAssPiece* LoadPiece(S3DModel* model, aiNode* node, const LuaTable& metaTable);
	void BuildPieceHierarchy(S3DModel* model);
    void CalculateRadius( S3DModel* model );
    void CalculateHeight( S3DModel* model );
    void CalculateMinMax( S3DModelPiece* piece );
};

#endif /* ASSPARSER_H */
