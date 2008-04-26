// 3DOParser.h: interface for the C3DOParser class.
//
//////////////////////////////////////////////////////////////////////
#ifndef SPRING_3DOPARSER_H
#define SPRING_3DOPARSER_H

#include <vector>
#include <string>
#include "float3.h"
#include "Rendering/Textures/TextureHandler.h"
#include <map>
#include <set>
#include "3DModelParser.h"

class CMatrix44f;
class CFileHandler;

struct S3DOVertex {
	float3 pos;
	float3 normal;
	std::vector<int> prims;
};

struct S3DOPrimitive {
	std::vector<int> vertices;
	std::vector<float3> normals;		//normals per vertex
	float3 normal;
	int numVertex;
	CTextureHandler::UnitTexture* texture;
};

struct S3DO {
	std::string name;
	std::vector<S3DO*> childs;
	std::vector<S3DOPrimitive> prims;
	std::vector<S3DOVertex> vertices;
	float3 offset;
	unsigned int displist;
	bool isEmpty;
	float radius;
	float3 relMidPos;
	float maxx,maxy,maxz;
	float minx,miny,minz;

	void DrawStatic();
	~S3DO();
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
	S3DOModel* Load3DO(std::string name, float scale = 1, int side = 1);
	// S3DOModel* Load3DO(std::string name,float scale,int side,const float3& offsets);
	LocalS3DOModel *CreateLocalModel(S3DOModel *model, std::vector<struct PieceInfo> *pieces);

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
	void CreateLocalModel(S3DO *model, LocalS3DOModel *lmodel, std::vector<struct PieceInfo> *pieces, int *piecenum);

	std::map<std::string, S3DOModel*> units;
	std::set<std::string> teamtex;

	int curOffset;
	unsigned char* fileBuf;
	void SimStreamRead(void* buf,int length);
};

#endif // SPRING_3DOPARSER_H
