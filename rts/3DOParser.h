// 3DOParser.h: interface for the C3DOParser class.
//
//////////////////////////////////////////////////////////////////////
#pragma warning(disable:4786)

#ifndef 3DOPARSER_H
#define 3DOPARSER_H

#include <vector>
#include <string>
#include "float3.h"
#include <iostream>
#include <fstream>
#include "TextureHandler.h"
#include <map>
#include <set>

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
	std::vector<float3> normals;		//normals per vertex
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
		int VersionSignature;
		int NumberOfVertices;
		int NumberOfPrimitives;
		int SelectionPrimitive;
		int XFromParent;
		int YFromParent;
		int ZFromParent;
		int OffsetToObjectName;
		int Always_0;
		int OffsetToVertexArray;
		int OffsetToPrimitiveArray;
		int OffsetToSiblingObject;
		int OffsetToChildObject;
	} _3DObject;

	typedef struct _Vertex
	{
		int x;
		int y;
		int z;
	} _Vertex;

	typedef struct _Primitive
	{
		int PaletteEntry;
		int NumberOfVertexIndexes;
		int Always_0;
		int OffsetToVertexIndexArray;
		int OffsetToTextureName;
		int Unknown_1;
		int Unknown_2;
		int Unknown_3;    
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

#endif // 3DOPARSER_H
