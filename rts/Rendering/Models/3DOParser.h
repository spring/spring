/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_3DOPARSER_H
#define SPRING_3DOPARSER_H

#include <vector>
#include <string>
#include "float3.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include <map>
#include <set>
#include "IModelParser.h"


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
	C3DOTextureHandler::UnitTexture* texture;
};

struct S3DOPiece: public S3DModelPiece {
	S3DOPiece() { parent = NULL; }

	void DrawList() const;
	void SetMinMaxExtends();
	int GetVertexCount() const { return vertices.size(); }
	const float3& GetVertexPos(int idx) const { return vertices[idx].pos; }
	void Shatter(float, int, int, const float3&, const float3&) const;

	std::vector<S3DOVertex> vertices;
	std::vector<S3DOPrimitive> prims;
	float radius;
	float3 relMidPos;
};

class C3DOParser: public IModelParser
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

public:
	C3DOParser();

	S3DModel* Load(const std::string& name);

private:
	void CalcNormals(S3DOPiece* o) const;
	void GetPrimitives(S3DOPiece* obj, int pos, int num, int excludePrim);
	void GetVertexes(_3DObject* o,S3DOPiece* object);
	std::string GetText(int pos);

	S3DOPiece* LoadPiece(S3DModel*, int, S3DOPiece*, int*);

	std::set<std::string> teamtex;

	int curOffset;
	unsigned char* fileBuf;
	void SimStreamRead(void* buf,int length);
};

#endif // SPRING_3DOPARSER_H
