#ifndef S3OPARSER_H
#define S3OPARSER_H
// s3oParser.h: interface for the Cs3oParser class.
//
//////////////////////////////////////////////////////////////////////

#include <map>
#include "IModelParser.h"


struct SS3OVertex {
	float3 pos;
	float3 normal;
	float textureX;
	float textureY;
};

struct SS3OPiece : public S3DModelPiece {
	const float3& GetVertexPos(const int& idx) const { return vertices[idx].pos; };

	std::vector<SS3OVertex> vertices;
	std::vector<unsigned int> vertexDrawOrder;
	int primitiveType;
};

class CS3OParser : public IModelParser
{
public:
	S3DModel* Load(std::string name);
	void Draw(S3DModelPiece *o);

private:
	SS3OPiece* LoadPiece(unsigned char* buf, int offset,S3DModel* model);
	void FindMinMax(SS3OPiece *object);
};

#endif /* S3OPARSER_H */
