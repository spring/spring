// 3DOParser.h: interface for the C3DOParser class.
//
//////////////////////////////////////////////////////////////////////
#ifndef __3DOPARSER_H__
#define __3DOPARSER_H__

#include "archdef.h"

#include <vector>
#include <string>
#include <map>
#include <set>
#include <iostream>
#include <fstream>
#include "float3.h"
#include "TextureHandler.h"

using namespace std;
class CMatrix44f;

class CFileHandler;

struct SVertex {
	float3 pos;
	float3 normal;
	std::vector<int> prims;
};

struct SPrimitive {
	std::vector<int> vertices;
	float3 normal;
	int numVertex;
	CTextureHandler::UnitTexture* texture;
};

struct S3DO {
	std::string name;
	std::vector<S3DO*> childs;
	std::vector<SPrimitive> prims;
	std::vector<SVertex> vertices;
	float3 offset;
	unsigned int displist;
	bool isEmpty;
	float radius;
	float3 relMidPos;
	float maxx,maxy,maxz;
	float minx,miny,minz;
	bool writeName;

	void DrawStatic();
	~S3DO();
};

struct S3DOModel
{
	S3DO* rootobject;
	int numobjects;
	float radius;
	float height;
	string name;
	int farTextureNum;
};

struct PieceInfo;

struct LocalS3DO
{
	float3 offset;
	unsigned int displist;
	string name;
	std::vector<LocalS3DO*> childs;
	LocalS3DO *parent;
	S3DO *original;
	PieceInfo *anim;
	void Draw();
	void GetPiecePosIter(CMatrix44f* mat);
};

struct LocalS3DOModel
{	
	int numpieces;
	//LocalS3DO *rootobject;
	LocalS3DO *pieces;
	int *scritoa;  //scipt index to local array index
	LocalS3DOModel(){};
	~LocalS3DOModel();
	void Draw();
	float3 GetPiecePos(int piecenum);
	CMatrix44f GetPieceMatrix(int piecenum);
	float3 GetPieceDirection(int piecenum);
};

class C3DOParser  
{
	typedef struct _3DObject
	{
		long VersionSignature;
		long NumberOfVertices;
		long NumberOfPrimitives;
		long SelectionPrimitive;
		long XFromParent;
		long YFromParent;
		long ZFromParent;
		long OffsetToObjectName;
		long Always_0;
		long OffsetToVertexArray;
		long OffsetToPrimitiveArray;
		long OffsetToSiblingObject;
		long OffsetToChildObject;
	} _3DObject;

	typedef struct _Vertex
	{
		long x;
		long y;
		long z;
	} _Vertex;

	typedef struct _Primitive
	{
		long PaletteEntry;
		long NumberOfVertexIndexes;
		long Always_0;
		long OffsetToVertexIndexArray;
		long OffsetToTextureName;
		long Unknown_1;
		long Unknown_2;
		long Unknown_3;    
	} _Primitive;

	typedef std::vector<float3> vertex_vector;

public:
	C3DOParser();
	virtual ~C3DOParser();
	S3DOModel* Load3DO(string name,float scale=1,int side=1);
	LocalS3DOModel *CreateLocalModel(S3DOModel *model, vector<struct PieceInfo> *pieces);
	
private:
	void FindCenter(S3DO* object);
	float FindRadius(S3DO* object,float3 offset);
	float FindHeight(S3DO* object,float3 offset);
	void CalcNormals(S3DO* o);

	void DeleteS3DO(S3DO* o);
	void CreateLists(S3DO* o);
	float scaleFactor;

	void GetPrimitives(S3DO* obj,int pos,int num,vertex_vector* vv,int excludePrim,int side);
	void GetVertexes(_3DObject* o,S3DO* object);
	std::string GetText(int pos);
	bool ReadChild(int pos,S3DO* root,int side, int *numobj);
	void DrawSub(S3DO* o);
	string C3DOParser::GetLine(CFileHandler& fh);
	void CreateLocalModel(S3DO *model, LocalS3DOModel *lmodel, vector<struct PieceInfo> *pieces, int *piecenum);
	
	map<string,S3DOModel*> units;
	set<string> teamtex;

	int curOffset;
	unsigned char* fileBuf;
	void SimStreamRead(void* buf,int length);
};

extern C3DOParser* unit3doparser;

#endif // __3DOPARSER_H__

