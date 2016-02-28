/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef SPRING_3DOPARSER_H
#define SPRING_3DOPARSER_H

#include "3DModel.h"
#include "IModelParser.h"

#include "System/float3.h"
#include "Rendering/Textures/3DOTextureHandler.h"

#include <vector>
#include <string>
#include <set>



struct S3DOVertex
{
	S3DOVertex(float3 p, float3 n, float2 t)
	: pos(p), normal(n), texCoord(t) {}

	float3 pos;
	float3 normal;
	float2 texCoord;
};


struct S3DOPrimitive
{
	std::vector<int>    indices;  ///< indices to S3DOPiece::vertexPos
	std::vector<float3> vnormals; ///< per-vertex normals

	// the raw normal for this primitive (-v0v1.cross(v0v2))
	// used iff we have less than 3 or more than 4 vertices
	float3 primNormal;

	C3DOTextureHandler::UnitTexture* texture;
};


struct S3DOPiece: public S3DModelPiece
{
	void UploadGeometryVBOs() override;
	void DrawForList() const override;

	unsigned int GetVertexCount() const override { return vboAttributes.GetSize(); }
	unsigned int GetVertexDrawIndexCount() const override { return vboIndices.GetSize(); }
	const float3& GetVertexPos(const int idx) const override { return vertexAttribs[idx].pos; }
	const float3& GetNormal(const int idx)    const override { return vertexAttribs[idx].normal; }
	const std::vector<unsigned>& GetVertexIndices() const override { return vertexIndices; }

	float3 GetEmitPos() const override { return emitPos; }
	float3 GetEmitDir() const override { return emitDir; }

	void BindVertexAttribVBOs() const override;
	void UnbindVertexAttribVBOs() const override;

public:
	void SetMinMaxExtends();
	void CalcNormals();

public:
	std::vector<float3>    vertexPos; //FIXME
	std::vector<S3DOPrimitive> prims;

	float3 emitPos;
	float3 emitDir;

	std::vector<S3DOVertex> vertexAttribs;
	std::vector<unsigned int> vertexIndices;
};


class C3DOParser: public IModelParser
{
public:
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
	S3DOPiece* LoadPiece(S3DModel* model, int pos, S3DOPiece* parent, int* numobj, const std::vector<unsigned char>& fileBuf);

	C3DOTextureHandler::UnitTexture* GetTexture(S3DOPiece* obj, _Primitive* p, const std::vector<unsigned char>& fileBuf) const;
	static bool IsBasePlate(S3DOPiece* obj, S3DOPrimitive* face);

	void GetPrimitives(S3DOPiece* obj, int pos, int num, int excludePrim, const std::vector<unsigned char>& fileBuf);
	void GetVertexes(_3DObject* o, S3DOPiece* object, const std::vector<unsigned char>& fileBuf);

private:
	std::set<std::string> teamtex;

};

#endif // SPRING_3DOPARSER_H
