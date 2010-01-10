#ifndef ASSPARSER_H
#define ASSPARSER_H

#include <map>
#include "IModelParser.h"
#include "float3.h"

struct aiNode;
struct aiScene;

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

	const float3& GetVertexPos(const int& idx) const { return vertices[idx].pos; }
	std::vector<SAssVertex> vertices;

	std::vector<unsigned int> vertexDrawOrder;
	// cannot store these in SAssVertex
	std::vector<float3> sTangents; // == T(angent) dirs
	std::vector<float3> tTangents; // == B(itangent) dirs
};


class CAssParser: public IModelParser
{
public:
	S3DModel* Load(std::string name); ///< Load a model
	void Draw( const S3DModelPiece* o) const; ///< Build displaylist for loaded model

private:
	SAssPiece* LoadPiece(aiNode* node, const aiScene* scene);
};

#endif /* ASSPARSER_H */
