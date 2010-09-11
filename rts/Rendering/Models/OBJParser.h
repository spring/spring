/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef OBJPARSER_HDR
#define OBJPARSER_HDR

#include <map>
#include "IModelParser.h"
#include "System/Vec2.h"

struct SOBJTriangle {
	int vIndices[3];   // index of 1st/2nd/3rd vertex
	int nIndices[3];   // index of 1st/2nd/3rd normal
	int tIndices[3];   // index of 1st/2nd/3rd txcoor
};

struct SOBJPiece: public S3DModelPiece {
public:
	SOBJPiece() {
		parent      = NULL;
		displist    = 0;
		isEmpty     = true;
		type        = MODELTYPE_OBJ;
		colvol      = NULL;
		mins        = ZeroVector;
		maxs        = ZeroVector;
	}
	~SOBJPiece() {
		vertices.clear();
		vnormals.clear();
		texcoors.clear();

		triangles.clear();

		sTangents.clear();
		tTangents.clear();
	}

	void DrawList() const;
	void SetMinMaxExtends();
	void SetVertexTangents();

	void SetVertexCount(int n) { vertices.resize(n); }
	void SetNormalCount(int n) { vnormals.resize(n); }
	void SetTxCoorCount(int n) { texcoors.resize(n); }

	void AddTriangle(const SOBJTriangle& t) { triangles.push_back(t); }
	void SetTriangle(int idx, const SOBJTriangle& t) { triangles[idx] = t; }
	const SOBJTriangle& GetTriangle(int idx) const { return triangles[idx]; }

	int GetTriangleCount() const { return (triangles.size()); }
	int GetVertexCount() const { return vertices.size(); }
	int GetNormalCount() const { return vnormals.size(); }
	int GetTxCoorCount() const { return texcoors.size(); }

	const float3& GetVertexPos(int idx) const { return GetVertex(idx); }
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
	std::vector<float3> vertices;
	std::vector<float3> vnormals;
	std::vector<float2> texcoors;

	std::vector<SOBJTriangle> triangles;

	std::vector<float3> sTangents;
	std::vector<float3> tTangents;
};

class LuaTable;
class COBJParser: public IModelParser {
public:
	S3DModel* Load(const std::string&);

private:
	typedef std::map<std::string, SOBJPiece*> PieceMap;

	bool ParseModelData(S3DModel*, const std::string&, const LuaTable&);
	bool BuildModelPieceTree(S3DModel*, const PieceMap&, const LuaTable&, bool, bool);
	void BuildModelPieceTreeRec(S3DModel*, SOBJPiece*, const PieceMap&, const LuaTable&, bool, bool);
};

#endif
