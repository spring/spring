/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_3DOPARSER_H
#define SPRING_3DOPARSER_H

#include <vector>
#include <string>
#include "System/float3.h"
#include "Rendering/Textures/3DOTextureHandler.h"
#include <map>
#include <set>
#include "IModelParser.h"


class CMatrix44f;

struct S3DOVertex {
	float3 pos;

	// summed over the primNormal's of all primitives we share
	float3 normal;

	std::vector<int> prims;
};

struct S3DOPrimitive {
	std::vector<int> vertices;
	std::vector<float3> vnormals; ///< per-vertex normals

	// the raw normal for this primitive (-v0v1.cross(v0v2))
	// used iff we have less than 3 or more than 4 vertices
	float3 primNormal;

	int numVertex;
	C3DOTextureHandler::UnitTexture* texture;
};

struct S3DOPiece: public S3DModelPiece {
	S3DOPiece() { parent = NULL; radius = 0; }

	void DrawForList() const;
	void SetMinMaxExtends();
	unsigned int GetVertexCount() const { return vertices.size(); }
	const float3& GetVertexPos(const int idx) const { return vertices[idx].pos; }
	const float3& GetNormal(const int idx) const { return vertices[idx].normal; }

	float3 GetPosOffset() const {
		//FIXME merge into float3 offset???
		float3 p = ZeroVector;

		// fix for 3DO *A units with two-vertex pieces
		if (vertices.size() == 2) {
			const S3DOVertex& v0 = vertices[0];
			const S3DOVertex& v1 = vertices[1];

			if (v0.pos.y > v1.pos.y) {
				p = float3(v0.pos.x, v0.pos.y, -v0.pos.z);
			} else {
				p = float3(v1.pos.x, v1.pos.y, -v1.pos.z);
			}
		}

		return p;
	}

	void Shatter(float pieceChance, int texType, int team, const float3& pos, const float3& speed) const;

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
	ModelType GetType() const { return MODELTYPE_3DO; }

private:
	void CalcNormals(S3DOPiece* o) const;
	void GetPrimitives(S3DOPiece* obj, int pos, int num, int excludePrim);
	void GetVertexes(_3DObject* o, S3DOPiece* object);
	std::string GetText(int pos);

	S3DOPiece* LoadPiece(S3DModel* model, int pos, S3DOPiece* parent,
			int* numobj);

	std::set<std::string> teamtex;

	int curOffset;
	unsigned char* fileBuf;
	void SimStreamRead(void* buf, int length);
};

#endif // SPRING_3DOPARSER_H
