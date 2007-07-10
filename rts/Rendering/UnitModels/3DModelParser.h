#ifndef SPRING_3DMODELPARSER_H
#define SPRING_3DMODELPARSER_H

#include <vector>
#include <string>
#include "Matrix44f.h"

struct S3DO;
struct SS3O;
class	C3DOParser;
class	CS3OParser;

using namespace std;

struct S3DOModel
{
	S3DO* rootobject3do;
	SS3O* rootobjects3o;
	int numobjects;
	float radius;
	float height;
	string name;
	int farTextureNum;
	float maxx,maxy,maxz;
	float minx,miny,minz;
	float3 relMidPos;
	int textureType;		//0=3do, otherwise s3o

	void DrawStatic();

};

struct PieceInfo;

struct LocalS3DO
{
	float3 offset;
	unsigned int displist;
	vector<unsigned int> lodDispLists;
	string name;
	std::vector<LocalS3DO*> childs;
	LocalS3DO *parent;
	S3DO *original3do;
	SS3O *originals3o;
	PieceInfo *anim;
	void Draw() const;
	void DrawLOD(unsigned int lod) const;
	void GetPiecePosIter(CMatrix44f* mat) const;
	void SetLODCount(unsigned int count);
};

struct LocalS3DOModel
{	
	int numpieces;
	//LocalS3DO *rootobject;
	LocalS3DO *pieces;
	int *scritoa;  //scipt index to local array index
	unsigned int lodCount;

	LocalS3DOModel() : lodCount(0) {};
	~LocalS3DOModel();
	void Draw() const;
	void DrawLOD(unsigned int lod) const;
	bool PieceExists(int piecenum) const;
	float3 GetPiecePos(int piecenum) const;
	CMatrix44f GetPieceMatrix(int piecenum) const;
	float3 GetPieceDirection(int piecenum) const;
	int GetPieceVertCount(int piecenum) const;

	//helper function for emit-sfx, get position and direction for a specific piece
	void GetEmitDirPos(int piecenum, float3 &pos, float3 &dir) const;
	void SetLODCount(unsigned int count);
};

class C3DModelParser
{
public:
	C3DModelParser(void);
	~C3DModelParser(void);

	S3DOModel* Load3DO(string name,float scale=1,int side=1);
	S3DOModel* Load3DO(string name,float scale,int side,const float3& offsets);	
	LocalS3DOModel *CreateLocalModel(S3DOModel *model, vector<struct PieceInfo> *pieces);

	C3DOParser* unit3doparser;
	CS3OParser* units3oparser;
};

extern C3DModelParser* modelParser;

#endif /* SPRING_3DMODELPARSER_H */
