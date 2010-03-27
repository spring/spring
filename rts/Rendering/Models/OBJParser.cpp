/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <cassert>
#include <sstream>
#include <boost/regex.hpp>

#include "OBJParser.h"
#include "Lua/LuaParser.h"
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
		throw content_error("[OBJParser] could not find model-file " + modelFileName);
	}
	if (!metaFile.FileExists()) {
		throw content_error("[OBJParser] could not find meta-file " + metaFileName);
	}


	LuaParser metaFileParser(metaFileName, SPRING_VFS_MOD_BASE, SPRING_VFS_ZIP);
	metaFileParser.Execute();

	if (!metaFileParser.IsValid()) {
		throw content_error("[OBJParser] failed to parse meta-file " + metaFileName);
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
		throw content_error("[OBJParser] failed to parse model-data " + modelFileName);
	}

	return NULL;
}

void COBJParser::Draw(const S3DModelPiece*) const
{
}



bool COBJParser::ParseModelData(S3DModel* model, const std::string& modelData, const LuaTable& metaData)
{
	static const boost::regex commentPattern("^#.*");
	static const boost::regex objectPattern("^o [ ]*[a-zA-Z0-9]+[ ]*");
	static const boost::regex vertexPattern("^v [ ]*[0-9]+\\.[0-9]+ [ ]*[0-9]+\\.[0-9]+ [ ]*[0-9]+\\.[0-9]+[ ]*");
	static const boost::regex normalPattern("^vn [ ]*[0-9]+\\.[0-9]+ [ ]*[0-9]+\\.[0-9]+ [ ]*[0-9]+\\.[0-9]+[ ]*");
	static const boost::regex txcoorPattern("^vt [ ]*[0-9]\\.[0-9]+ [ ]*[0-9]\\.[0-9]+[ ]*");
	static const boost::regex polygonPattern(
		"^f "
		"[ ]*-?[0-9]+/-?[0-9]+/-?[0-9]+"
		"[ ]*-?[0-9]+/-?[0-9]+/-?[0-9]+"
		"[ ]*-?[0-9]+/-?[0-9]+/-?[0-9]+"
		// do not allow spaces around the '/' separators or the
		// stringstream >> operator will tokenize the wrong way
		// (according to the OBJ spec they are illegal anyway)
		// "[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+"      // 1st vertex/texcoor/normal idx
		// "[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+"      // 2nd vertex/texcoor/normal idx
		// "[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+"      // 3rd vertex/texcoor/normal idx
		"[ ]*"
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
						n += 1;

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

						assert(triangle.vIndices[n] < piece->vertexCount);
						assert(triangle.tIndices[n] < piece->vertexCount);
						assert(triangle.nIndices[n] < piece->vertexCount);
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

	model->rootobject = pieces["rootpiece"];

	if (model->rootobject != NULL) {
		BuildModelPieceTree(model->rootobject, pieces, metaData.SubTable("rootpiece"));
		return true;
	} else {
		for (std::map<std::string, SOBJPiece*>::iterator it = pieces.begin(); it != pieces.end(); ++it) {
			delete (it->second);
		}
	}

	return false;
}

void COBJParser::BuildModelPieceTree(S3DModelPiece* piece, const std::map<std::string, SOBJPiece*>& pieces, const LuaTable& pieceTable)
{
	piece->isEmpty = (piece->vertexCount == 0);
	piece->mins = ZeroVector;
	piece->maxs = ZeroVector;
	piece->offset = pieceTable.GetFloat3("offset", ZeroVector);
	piece->colvol = new CollisionVolume("box", ZeroVector, ZeroVector, COLVOL_TEST_CONT);
	piece->colvol->Disable();

	/*
	model = {
		rootpiece = {
			upperbody = {
				head = { offset = {0.0, 2.71828, 0.0}, },
				lshoulder = {
					offset = {-3.14159, 0.0, 0.0},
					lupperarm = {
						llowerarm = {
							lhand = {},
						},
					},
				},
				rshoulder = {
					offset = { 3.14159, 0.0, 0.0},
					rupperarm = {
						rlowerarm = {
							rhand = {},
						},
					},
				},
			},
			lowerbody = {},
		},

		radius = 123.0,
		height = 456.0,
		midpos = {0.0, 0.0, 0.0},
		tex1 = "tex1.png",
		tex2 = "tex2.png",
	}
	*/

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
			throw content_error("[OBJParser] meta-data piece named " + (*it) + " not defined in model");
		} else {
			SOBJPiece* childPiece = pieceIt->second;

			// this is not needed
			// childPiece->parent = piece;
			assert((*it) == childPiece->name);

			piece->childs.push_back(childPiece);
			BuildModelPieceTree(childPiece, pieces, pieceTable.SubTable(childPiece->name));
		}
	}
}
