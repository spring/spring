#ifndef ASSPARSER_H
#define ASSPARSER_H

#include <map>
#include "IModelParser.h"
#include "float3.h"

struct aiNode;

struct SAssPiece: public S3DModelPiece
{
public:
	aiNode* node;
	float3 pos;
	const float3& GetVertexPos(const int& idx) const { return pos; }
};


class CAssParser: public IModelParser
{
public:
	S3DModel* Load(std::string name); ///< Load a model
	void Draw( const S3DModelPiece* o) const; ///< Build displaylist for loaded model

private:
	SAssPiece* LoadPiece(aiNode* node);
};

#endif /* ASSPARSER_H */
