/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef OBJPARSER_HDR
#define OBJPARSER_HDR

#include <map>
#include "IModelParser.h"

struct SOBJTriangle {
	int vIndices[3];   // index of 1st/2nd/3rd vertex
	int nIndices[3];   // index of 1st/2nd/3rd normal
	int tIndices[3];   // index of 1st/2nd/3rd txcoor
};

struct SOBJPiece: public S3DModelPiece {
public:
	SOBJPiece() {
		vertexCount = 0;
		displist    = 0;
		isEmpty     = true;
		type        = MODELTYPE_OBJ;
	}

	void AddVertex(const float3& v) { vertices.push_back(v); vertexCount += 1; }
	void AddNormal(const float3& n) { vnormals.push_back(n);                   }
	void AddTxCoor(const float2& t) { texcoors.push_back(t);                   }
	void AddTriangle(const SOBJTriangle& t) { triangles.push_back(t); }

	const float3& GetVertexPos(const int& vIdx) const {
		return vertices[vIdx];
	}

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
	void Draw(const S3DModelPiece*) const;

private:
	bool ParseModelData(S3DModel*, const std::string&, const LuaTable&);
	void BuildModelPieceTree(S3DModelPiece*, const std::map<std::string, SOBJPiece*>&, const LuaTable&);
};

#endif
