/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <cassert>
#include <sstream>
#include <boost/regex.hpp>

#include "OBJParser.h"
#include "Lua/LuaParser.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Exceptions.h"
#include "System/FileSystem/FileHandler.h"

S3DModel* COBJParser::Load(const std::string& modelFileName)
{
	std::string metaFileName(modelFileName + ".lua");

	CFileHandler modelFile(modelFileName);
	CFileHandler metaFile(metaFileName);

	if (!modelFile.FileExists()) {
		throw content_error("[OBJParser] could not find model-file \"" + modelFileName + "\"");
	}
	if (!metaFile.FileExists()) {
		throw content_error("[OBJParser] could not find meta-file \"" + metaFileName + "\"");
	}


	LuaParser metaFileParser(metaFileName, SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	metaFileParser.Execute();

	if (!metaFileParser.IsValid()) {
		throw content_error("[OBJParser] failed to parse meta-file \"" + metaFileName + "\"");
	}

	// get the (root-level) model table
	const LuaTable& modelTable = metaFileParser.GetRoot();

	S3DModel* model = new S3DModel();
		model->name = modelFileName;
		model->type = MODELTYPE_OBJ;
		model->textureType = 0;
		model->numobjects = 0;
		model->rootobject = NULL;
		model->radius = modelTable.GetFloat("radius", 0.0f);
		model->height = modelTable.GetFloat("height", 0.0f);
		model->relMidPos = modelTable.GetFloat3("midpos", ZeroVector);
		model->tex1 = modelTable.GetString("tex1", "");
		model->tex2 = modelTable.GetString("tex2", "");
		model->mins = ZeroVector; // needed for per-piece coldet only?
		model->maxs = ZeroVector; // needed for per-piece coldet only?

	// basic S3O-style texturing
	texturehandlerS3O->LoadS3OTexture(model);

	std::string modelData;
	modelFile.LoadStringData(modelData);

	if (ParseModelData(model, modelData, modelTable)) {
		assert(model->numobjects == modelTable.GetInt("numpieces", 0));
		return model;
	} else {
		delete model;
		throw content_error("[OBJParser] failed to parse model-data \"" + modelFileName + "\"");
	}

	return NULL;
}

void COBJParser::Draw(const S3DModelPiece* piece) const
{
	if (piece->isEmpty) {
		return;
	}

	const SOBJPiece* objPiece = dynamic_cast<const SOBJPiece*>(piece);

	CVertexArray* va = GetVertexArray();
		va->Initialize();
		va->EnlargeArrays(objPiece->GetTriangleCount() * 3, 0, VA_SIZE_TN);

	for (int i = objPiece->GetTriangleCount() - 1; i >= 0; i--) {
		const SOBJTriangle& tri = objPiece->GetTriangle(i);
		const float3&
			v0p = objPiece->GetVertex(tri.vIndices[0]),
			v1p = objPiece->GetVertex(tri.vIndices[1]),
			v2p = objPiece->GetVertex(tri.vIndices[2]);
		const float3&
			v0n = objPiece->GetNormal(tri.nIndices[0]),
			v1n = objPiece->GetNormal(tri.nIndices[1]),
			v2n = objPiece->GetNormal(tri.nIndices[2]);
		/*
		// TODO: these need a new CVertexArray::AddVertex variant
		const float3&
			v0st = objPiece->GetSTangent(tri.vIndices[0]);
			v1st = objPiece->GetSTangent(tri.vIndices[1]);
			v2st = objPiece->GetSTangent(tri.vIndices[2]);
			v0tt = objPiece->GetTTangent(tri.vIndices[0]);
			v1tt = objPiece->GetTTangent(tri.vIndices[1]);
			v2tt = objPiece->GetTTangent(tri.vIndices[2]);
		*/
		const float2&
			v0tc = objPiece->GetTxCoor(tri.tIndices[0]),
			v1tc = objPiece->GetTxCoor(tri.tIndices[1]),
			v2tc = objPiece->GetTxCoor(tri.tIndices[2]);

		va->AddVertexQTN(v0p, v0tc.x, v0tc.y, v0n);
		va->AddVertexQTN(v1p, v1tc.x, v1tc.y, v1n);
		va->AddVertexQTN(v2p, v2tc.x, v2tc.y, v2n);
	}

	va->DrawArrayTN(GL_TRIANGLES);
}



bool COBJParser::ParseModelData(S3DModel* model, const std::string& modelData, const LuaTable& metaData)
{
	static const boost::regex commentPattern("^[ ]*(#|//).*");
	static const boost::regex objectPattern("^[ ]*o [ ]*[a-zA-Z0-9_]+[ ]*");
	static const boost::regex vertexPattern("^[ ]*v [ ]*-?[0-9]+\\.[0-9]+ [ ]*-?[0-9]+\\.[0-9]+ [ ]*-?[0-9]+\\.[0-9]+[ ]*");
	static const boost::regex normalPattern("^[ ]*vn [ ]*-?[0-9]\\.[0-9]+ [ ]*-?[0-9]\\.[0-9]+ [ ]*-?[0-9]\\.[0-9]+[ ]*");
	static const boost::regex txcoorPattern("^[ ]*vt [ ]*-?[0-9]\\.[0-9]+ [ ]*-?[0-9]\\.[0-9]+[ ]*");
	static const boost::regex polygonPattern(
		"^[ ]*f "
		"[ ]*-?[0-9]+/-?[0-9]+/-?[0-9]+"
		"[ ]*-?[0-9]+/-?[0-9]+/-?[0-9]+"
		"[ ]*-?[0-9]+/-?[0-9]+/-?[0-9]+"
		"[ ]*"
		// do not allow spaces around the '/' separators or the
		// stringstream >> operator will tokenize the wrong way
		// (according to the OBJ spec they are illegal anyway)
		// "[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+"      // 1st vertex/texcoor/normal idx
		// "[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+"      // 2nd vertex/texcoor/normal idx
		// "[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+"      // 3rd vertex/texcoor/normal idx
	);

	bool match = false;


	std::string pieceName;
	std::map<std::string, SOBJPiece*> pieces;

	SOBJPiece* piece = NULL;


	std::string line, lineHeader;
	std::stringstream lineStream;

	size_t prevReadIdx = 0;
	size_t currReadIdx = 0;

	while ((currReadIdx = modelData.find_first_of('\n', prevReadIdx)) != std::string::npos) {
		line = modelData.substr(prevReadIdx, (currReadIdx - prevReadIdx));

		if (!line.empty()) {
			              match = (boost::regex_match(line, commentPattern));
			if (!match) { match = (boost::regex_match(line, objectPattern )); }
			if (!match) { match = (boost::regex_match(line, vertexPattern )); }
			if (!match) { match = (boost::regex_match(line, normalPattern )); }
			if (!match) { match = (boost::regex_match(line, txcoorPattern )); }
			if (!match) { match = (boost::regex_match(line, polygonPattern)); }

			if (!match) {
				// ignore groups ('g'), smoothing groups ('s'),
				// and materials ("mtllib", "usemtl") for now
				prevReadIdx = currReadIdx + 1;
				continue;
			}

			lineStream.str(line);
			lineStream >> lineHeader;

			switch (lineHeader[0]) {
				case '#': {
					// comment, no-op
				} break;

				case 'o': {
					// named object (piece)
					lineStream >> pieceName;

					assert(pieces.find(pieceName) == pieces.end());

					piece = new SOBJPiece();
					piece->name = pieceName;
					pieces[pieceName] = piece;

					model->numobjects += 1;
				} break;

				case 'v': {
					// position, normal, or texture-coordinates
					// note: assume all normals are unit-length
					assert(piece != NULL);

					float x = 0.0f; lineStream >> x;
					float y = 0.0f; lineStream >> y;
					float z = 0.0f; lineStream >> z;

					     if (lineHeader == "v" ) { piece->AddVertex(float3(x, y, z)); }
					else if (lineHeader == "vn") { piece->AddNormal(float3(x, y, z)); }
					else if (lineHeader == "vt") { piece->AddTxCoor(float2(x, y   )); }
				} break;

				case 'f': {
					// face definition
					assert(piece != NULL);

					int
						vIdx = 0,
						tIdx = 0,
						nIdx = 0;
					size_t
						i = 0,
						j = 0,
						n = 0;

					SOBJTriangle triangle;

					while (lineStream.good() && n < 3) {
						std::string vtnIndices; lineStream >> vtnIndices;

						i = vtnIndices.find('/',     0); assert(i != std::string::npos);
						j = vtnIndices.find('/', i + 1); assert(j != std::string::npos);

						vIdx = std::atoi(vtnIndices.substr(    0,     i).c_str());
						tIdx = std::atoi(vtnIndices.substr(i + 1, j - i).c_str());
						nIdx = std::atoi(vtnIndices.substr(j + 1       ).c_str());

						if (vIdx < 0) {
							triangle.vIndices[n] = piece->vertexCount + vIdx;
						} else {
							triangle.vIndices[n] = vIdx - 1;
						}

						if (tIdx < 0) {
							triangle.tIndices[n] = piece->vertexCount + tIdx;
						} else {
							triangle.tIndices[n] = tIdx - 1;
						}

						if (nIdx < 0) {
							triangle.nIndices[n] = piece->vertexCount + nIdx;
						} else {
							triangle.nIndices[n] = nIdx - 1;
						}

						assert(triangle.vIndices[n] >= 0 && triangle.vIndices[n] < piece->vertexCount);
						assert(triangle.tIndices[n] >= 0 && triangle.tIndices[n] < piece->vertexCount);
						assert(triangle.nIndices[n] >= 0 && triangle.nIndices[n] < piece->vertexCount);
						n += 1;
					}

					assert(n == 3);
					piece->AddTriangle(triangle);
				} break;

				default: {
				} break;
			}

			lineStream.clear();
			lineStream.str("");
		}

		prevReadIdx = currReadIdx + 1;
	}

	if (BuildModelPieceTree(model, pieces, metaData.SubTable("pieces"))) {
		return true;
	}

	for (std::map<std::string, SOBJPiece*>::iterator it = pieces.begin(); it != pieces.end(); ++it) {
		delete (it->second);
	}

	throw content_error("[OBJParser] model " + model->name + " has no uniquely defined root-piece");
	return false;
}


bool COBJParser::BuildModelPieceTree(S3DModel* model, const std::map<std::string, SOBJPiece*>& pieces, const LuaTable& piecesTable)
{
	std::vector<std::string> rootPieceNames;
	piecesTable.GetKeys(rootPieceNames);

	if (rootPieceNames.size() != 1) {
		return false;
	}

	const std::string& rootPieceName = rootPieceNames[0];
	const std::map<std::string, SOBJPiece*>::const_iterator rootPieceIt = pieces.find(rootPieceName);

	if (rootPieceIt != pieces.end()) {
		model->rootobject = rootPieceIt->second;
		BuildModelPieceTreeRec(model->rootobject, pieces, piecesTable.SubTable(rootPieceName));
		return true;
	}

	return false;
}

void COBJParser::BuildModelPieceTreeRec(S3DModelPiece* piece, const std::map<std::string, SOBJPiece*>& pieces, const LuaTable& pieceTable)
{
	piece->isEmpty = (piece->vertexCount == 0);
	piece->mins = ZeroVector;
	piece->maxs = ZeroVector;
	piece->offset = pieceTable.GetFloat3("offset", ZeroVector);
	piece->colvol = new CollisionVolume("box", ZeroVector, ZeroVector, COLVOL_TEST_CONT);
	piece->colvol->Disable();

	piece->SetVertexTangents();

	std::vector<std::string> childPieceNames;
	pieceTable.GetKeys(childPieceNames);

	for (std::vector<std::string>::const_iterator it = childPieceNames.begin(); it != childPieceNames.end(); ++it) {
		// NOTE: handle this better? can't check types
		// (both tables), can't reliably check lengths,
		// test for presence of float keys?
		if ((*it) == "offset") {
			continue;
		}

		std::map<std::string, SOBJPiece*>::const_iterator pieceIt = pieces.find(*it);

		if (pieceIt == pieces.end()) {
			throw content_error("[OBJParser] meta-data piece named \"" + (*it) + "\" not defined in model");
		} else {
			SOBJPiece* childPiece = pieceIt->second;

			// childPiece->parent = piece;
			assert((*it) == childPiece->name);

			piece->childs.push_back(childPiece);
			BuildModelPieceTreeRec(childPiece, pieces, pieceTable.SubTable(childPiece->name));
		}
	}
}






void SOBJPiece::SetVertexTangents()
{
	if (isEmpty) {
		return;
	}

	sTangents.resize(vertexCount, ZeroVector);
	tTangents.resize(vertexCount, ZeroVector);

	// set the triangle-level S- and T-tangents
	for (int i = GetTriangleCount() - 1; i >= 0; i--) {
		const SOBJTriangle& tri = GetTriangle(i);
		const float3&
			p0 = GetVertex(tri.vIndices[0]),
			p1 = GetVertex(tri.vIndices[1]),
			p2 = GetVertex(tri.vIndices[2]);
		const float2&
			tc0 = GetTxCoor(tri.tIndices[0]),
			tc1 = GetTxCoor(tri.tIndices[1]),
			tc2 = GetTxCoor(tri.tIndices[2]);

		const float x1x0 = p1.x - p0.x, x2x0 = p2.x - p0.x;
		const float y1y0 = p1.y - p0.y, y2y0 = p2.y - p0.y;
		const float z1z0 = p1.z - p0.z, z2z0 = p2.z - p0.z;

		const float s1 = tc1.x - tc0.x, s2 = tc2.x - tc0.x;
		const float t1 = tc1.y - tc0.y, t2 = tc2.y - tc0.y;

		// if d is 0, texcoors are degenerate
		const float d = (s1 * t2 - s2 * t1);
		const bool  b = (d > -0.0001f && d < 0.0001f);
		const float r = b? 1.0f: 1.0f / d;

		// note: not necessarily orthogonal to each other
		// or to vertex normal (only to the triangle plane)
		const float3 sdir((t2 * x1x0 - t1 * x2x0) * r, (t2 * y1y0 - t1 * y2y0) * r, (t2 * z1z0 - t1 * z2z0) * r);
		const float3 tdir((s1 * x2x0 - s2 * x1x0) * r, (s1 * y2y0 - s2 * y1y0) * r, (s1 * z2z0 - s2 * z1z0) * r);

		sTangents[tri.vIndices[0]] += sdir;
		sTangents[tri.vIndices[1]] += sdir;
		sTangents[tri.vIndices[2]] += sdir;

		tTangents[tri.vIndices[0]] += tdir;
		tTangents[tri.vIndices[1]] += tdir;
		tTangents[tri.vIndices[2]] += tdir;
	}

	// set the smoothed per-vertex tangents
	for (int vrtIdx = vertices.size() - 1; vrtIdx >= 0; vrtIdx--) {
		float3& n = vnormals[vrtIdx];
		float3& s = sTangents[vrtIdx];
		float3& t = tTangents[vrtIdx];
		int h = 1;

		if (s == ZeroVector) { s = float3(1.0f, 0.0f, 0.0f); }
		if (t == ZeroVector) { t = float3(0.0f, 1.0f, 0.0f); }

		h = ((n.cross(s)).dot(t) < 0.0f)? -1: 1;
		s = (s - n * n.dot(s));
		s = s.SafeANormalize();
		t = (s.cross(n)) * h;
	}
}
