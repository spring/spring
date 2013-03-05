/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef OBJ_PARSER_H
#define OBJ_PARSER_H

#include <map>
#include "IModelParser.h"
#include "System/Vec2.h"

struct SOBJTriangle {
	int vIndices[3]; ///< index of 1st/2nd/3rd vertex
	int nIndices[3]; ///< index of 1st/2nd/3rd normal
	int tIndices[3]; ///< index of 1st/2nd/3rd txcoor
};

struct SOBJPiece: public S3DModelPiece {
public:
	SOBJPiece() {
		parent      = NULL;
		dispListID  = 0;
		isEmpty     = true;
		type        = MODELTYPE_OBJ;
		mins        = ZeroVector;
		maxs        = ZeroVector;
	}

	void UploadGeometryVBOs();
	void DrawForList() const;
	void SetMinMaxExtends();
	void SetVertexTangents();

	void SetVertexCount(unsigned int n) { vertices.resize(n); }
	void SetNormalCount(unsigned int n) { vnormals.resize(n); }
	void SetTxCoorCount(unsigned int n) { texcoors.resize(n); }

	void AddTriangle(const SOBJTriangle& t) { triangles.push_back(t); }
	void SetTriangle(int idx, const SOBJTriangle& t) { triangles[idx] = t; }
	const SOBJTriangle& GetTriangle(int idx) const { return triangles[idx]; }

	unsigned int GetTriangleCount() const { return (triangles.size()); }
	unsigned int GetVertexCount() const { return vertices.size(); }
	unsigned int GetNormalCount() const { return vnormals.size(); }
	unsigned int GetTxCoorCount() const { return texcoors.size(); }

	const float3& GetVertexPos(const int idx) const { return GetVertex(idx); }
	const float3& GetVertex(const int idx) const { return vertices[idx]; }
	const float3& GetNormal(const int idx) const { return vnormals[idx]; }
	const float2& GetTxCoor(const int idx) const { return texcoors[idx]; }
	const float3& GetSTangent(const int idx) const { return sTangents[idx]; }
	const float3& GetTTangent(const int idx) const { return tTangents[idx]; }

	void SetVertex(int idx, const float3& v) { vertices[idx] = v; }
	void SetNormal(int idx, const float3& v) { vnormals[idx] = v; }
	void SetTxCoor(int idx, const float2& v) { texcoors[idx] = v; }
	void SetSTangent(int idx, const float3& v) { sTangents[idx] = v; }
	void SetTTangent(int idx, const float3& v) { tTangents[idx] = v; }

	void AddVertex(const float3& v) { vertices.push_back(v); }
	void AddNormal(const float3& v) { vnormals.push_back(v); }
	void AddTxCoor(const float2& v) { texcoors.push_back(v); }

private:
	VBO vboPositions;
	VBO vboNormals;
	VBO vboTexcoords;
	VBO vbosTangents;
	VBO vbotTangents;

	std::vector<float3> vertices;
	std::vector<float3> vnormals;
	std::vector<float2> texcoors;

	std::vector<SOBJTriangle> triangles;
	std::vector<unsigned int> vertexDrawIndices;

	std::vector<float3> sTangents;
	std::vector<float3> tTangents;
};

class LuaTable;
class COBJParser: public IModelParser {
public:
	S3DModel* Load(const std::string& modelFileName);
	ModelType GetType() const { return MODELTYPE_OBJ; }

private:
	typedef std::map<std::string, SOBJPiece*> PieceMap;

	bool ParseModelData(
			S3DModel* model,
			const std::string& modelData,
			const LuaTable& metaData);
	bool BuildModelPieceTree(
			S3DModel* model,
			const PieceMap& pieceMap,
			const LuaTable& piecesTable,
			bool globalVertexOffsets,
			bool localPieceOffsets);
	void BuildModelPieceTreeRec(
			S3DModel* model,
			SOBJPiece* piece,
			const PieceMap& pieceMap,
			const LuaTable& pieceTable,
			bool globalVertexOffsets,
			bool localPieceOffsets);
};

#endif // OBJ_PARSER_H
