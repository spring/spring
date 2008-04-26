#ifndef S3OPARSER_H
#define S3OPARSER_H
// s3oParser.h: interface for the Cs3oParser class.
//
//////////////////////////////////////////////////////////////////////

#include "3DModelParser.h"
#include <map>

struct SS3O;

struct SS3OVertex {
	float3 pos;
	float3 normal;
	float textureX;
	float textureY;
};

struct SS3O {
	std::string name;
	std::vector<SS3O*> childs;
	std::vector<SS3OVertex> vertices;
	std::vector<unsigned int> vertexDrawOrder;
	float3 offset;
	int primitiveType;
	unsigned int displist;
	bool isEmpty;
	float maxx,maxy,maxz;
	float minx,miny,minz;

	void DrawStatic();
	~SS3O();
};

class CS3OParser
{
public:
	CS3OParser();
	virtual ~CS3OParser();

	S3DOModel* LoadS3O(std::string name, float scale = 1, int side = 1);
	LocalS3DOModel *CreateLocalModel(S3DOModel *model, std::vector<struct PieceInfo> *pieces);

private:
	SS3O* LoadPiece(unsigned char* buf, int offset,S3DOModel* model);
	void DeleteSS3O(SS3O* o);
	void FindMinMax(SS3O *object);
	void DrawSub(SS3O* o);
	void CreateLists(SS3O *o);
	void CreateLocalModel(SS3O *model, LocalS3DOModel *lmodel, std::vector<struct PieceInfo> *pieces, int *piecenum);

	std::map<std::string,S3DOModel*> units;
};

#endif /* S3OPARSER_H */
