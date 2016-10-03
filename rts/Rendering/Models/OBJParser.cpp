/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <boost/regex.hpp>

#include "OBJParser.h"

#include "Lua/LuaParser.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Exceptions.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Util.h"

#include <cassert>
#include <sstream>

#define LOG_SECTION_OBJ_PARSER "OBJParser"
LOG_REGISTER_SECTION_GLOBAL(LOG_SECTION_OBJ_PARSER)

// use the specific section for all LOG*() calls in this source file
#ifdef LOG_SECTION_CURRENT
	#undef LOG_SECTION_CURRENT
#endif
#define LOG_SECTION_CURRENT LOG_SECTION_OBJ_PARSER

S3DModel* COBJParser::Load(const std::string& modelFileName)
{
	std::string metaFileName = modelFileName.substr(0, modelFileName.find_last_of('.')) + ".lua";

	CFileHandler modelFile(modelFileName);
	CFileHandler metaFile(metaFileName);

	if (!modelFile.FileExists())
		throw content_error("[OBJParser] could not find model-file \"" + modelFileName + "\"");

	if (!metaFile.FileExists())
		throw content_error("[OBJParser] could not find meta-file \"" + metaFileName + "\"");


	LuaParser metaFileParser(metaFileName, SPRING_VFS_ZIP, SPRING_VFS_ZIP);
	metaFileParser.Execute();

	if (!metaFileParser.IsValid())
		throw content_error("[OBJParser] failed to parse meta-file \"" + metaFileName + "\"");


	// get the (root-level) model table
	const LuaTable& modelTable = metaFileParser.GetRoot();

	S3DModel* model = new S3DModel();
		model->name = modelFileName;
		model->type = MODELTYPE_OBJ;
		model->textureType = 0;
		model->numPieces = 0;
		model->mins = DEF_MIN_SIZE;
		model->maxs = DEF_MAX_SIZE;
		model->texs[0] = modelTable.GetString("tex1", "");
		model->texs[1] = modelTable.GetString("tex2", "");

	// basic S3O-style texturing
	texturehandlerS3O->PreloadTexture(model);

	std::string modelData;
	modelFile.LoadStringData(modelData);

	if (ParseModelData(model, modelData, modelTable)) {
		// set after the extrema are known
		model->radius = modelTable.GetFloat("radius", (model->maxs   - model->mins  ).Length() * 0.5f);
		model->height = modelTable.GetFloat("height", (model->maxs.y - model->mins.y)                );
		model->relMidPos = modelTable.GetFloat3("midpos", (model->maxs + model->mins) * 0.5f);

		assert(model->numPieces == modelTable.GetInt("numpieces", 0));
		return model;
	} else {
		delete model;
		throw content_error("[OBJParser] failed to parse model-data \"" + modelFileName + "\"");
	}

	return nullptr;
}

bool COBJParser::ParseModelData(S3DModel* model, const std::string& modelData, const LuaTable& metaData)
{
	static const boost::regex commentPattern("^[ ]*(#|//).*");
	static const boost::regex objectPattern("^[ ]*o [ ]*[a-zA-Z0-9_]+[ ]*");
	static const boost::regex vertexPattern(
		"^[ ]*v "
		"[ ]*-?[0-9]*\\.?[0-9]*(e-?[0-9]*)? "
		"[ ]*-?[0-9]*\\.?[0-9]*(e-?[0-9]*)? "
		"[ ]*-?[0-9]*\\.?[0-9]*(e-?[0-9]*)?"
		"[ ]*"
	);
	static const boost::regex normalPattern(
		"^[ ]*vn "
		"[ ]*-?[0-9]*\\.?[0-9]*(e-?[0-9]*)? "
		"[ ]*-?[0-9]*\\.?[0-9]*(e-?[0-9]*)? "
		"[ ]*-?[0-9]*\\.?[0-9]*(e-?[0-9]*)?"
		"[ ]*"
	);
	static const boost::regex txcoorPattern(
		"^[ ]*vt "
		"[ ]*-?[0-9]*\\.?[0-9]*(e-?[0-9]*)? "
		"[ ]*-?[0-9]*\\.?[0-9]*(e-?[0-9]*)?"
		"[ ]*"
	);
	static const boost::regex polygonPattern(
		"^[ ]*f "
		"[ ]*-?[0-9]+/-?[0-9]+/-?[0-9]+"
		"[ ]*-?[0-9]+/-?[0-9]+/-?[0-9]+"
		"[ ]*-?[0-9]+/-?[0-9]+/-?[0-9]+"
		"[ ]*"
		// do not allow spaces around the '/' separators or the
		// stringstream >> operator will tokenize the wrong way
		// (according to the OBJ spec they are illegal anyway:
		// "there is no space between numbers and the slashes")
		// "[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+"      // 1st vertex/texcoor/normal idx
		// "[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+"      // 2nd vertex/texcoor/normal idx
		// "[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+[ ]*/[ ]*-?[0-9]+"      // 3rd vertex/texcoor/normal idx
	);


	PieceMap pieceMap;
	SOBJPiece* piece = nullptr;

	std::vector<float3> vertices; vertices.reserve(2048);
	std::vector<float2> texcoors; texcoors.reserve(2048);
	std::vector<float3> vnormals; vnormals.reserve(2048);

	std::string line, lineHeader;
	std::stringstream lineStream;

	size_t prevReadIdx = 0;
	size_t currReadIdx = 0;

	bool regexMatch = false;

	while ((currReadIdx = modelData.find_first_of('\n', prevReadIdx)) != std::string::npos) {
		line = modelData.substr(prevReadIdx, (currReadIdx - prevReadIdx));
		line = StringReplaceInPlace(line, '\r', ' ');

		if (!line.empty()) {
			                   regexMatch = (boost::regex_match(line, commentPattern));
			if (!regexMatch) { regexMatch = (boost::regex_match(line, objectPattern )); }
			if (!regexMatch) { regexMatch = (boost::regex_match(line, vertexPattern )); }
			if (!regexMatch) { regexMatch = (boost::regex_match(line, normalPattern )); }
			if (!regexMatch) { regexMatch = (boost::regex_match(line, txcoorPattern )); }
			if (!regexMatch) { regexMatch = (boost::regex_match(line, polygonPattern)); }

			if (!regexMatch) {
				// ignore groups ('g'), smoothing groups ('s'),
				// and materials ("mtllib", "usemtl") for now
				// (s-groups are obsolete with vertex normals)
				LOG_L(L_WARNING, "Failed to parse line \"%s\" for model \"%s\"", line.c_str(), model->name.c_str());

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
					std::string pieceName;
					lineStream >> pieceName;

					assert(pieceMap.find(pieceName) == pieceMap.end());

					piece = new SOBJPiece();
					piece->name = pieceName;
					pieceMap[pieceName] = piece;

					model->numPieces += 1;
				} break;

				case 'v': {
					// position, normal, or texture-coordinates
					assert(piece != nullptr);

					float3 f3;
						lineStream >> f3.x;
						lineStream >> f3.y;
						lineStream >> f3.z;
					float2 f2(f3.x, f3.y);

					     if (lineHeader == "v" ) { vertices.push_back(f3);   }
					else if (lineHeader == "vt") { texcoors.push_back(f2);   }
					else if (lineHeader == "vn") { vnormals.push_back(f3);   }
				} break;

				case 'f': {
					// face definition
					assert(piece != nullptr);

					int
						vIdx = 0,
						tIdx = 0,
						nIdx = 0;
					size_t
						i = 0,
						j = 0,
						n = 0;

					SOBJTriangle triangle = {{0, 0, 0}, {0, 0, 0}, {0, 0, 0}};

					while (lineStream.good() && n < 3) {
						std::string vtnIndices; lineStream >> vtnIndices;

						// vIdx/tcIdx/nIdx triplet of indices into the global lists
						i = vtnIndices.find('/',     0); assert(i != std::string::npos);
						j = vtnIndices.find('/', i + 1); assert(j != std::string::npos);

						vIdx = std::atoi(vtnIndices.substr(    0,     i).c_str());
						tIdx = std::atoi(vtnIndices.substr(i + 1, j - i).c_str());
						nIdx = std::atoi(vtnIndices.substr(j + 1       ).c_str());

						if (vIdx < 0) {
							triangle.vIndices[n] = vertices.size() + vIdx;
						} else {
							triangle.vIndices[n] = vIdx - 1;
						}

						if (tIdx < 0) {
							triangle.tIndices[n] = texcoors.size() + tIdx;
						} else {
							triangle.tIndices[n] = tIdx - 1;
						}

						if (nIdx < 0) {
							triangle.nIndices[n] = vnormals.size() + nIdx;
						} else {
							triangle.nIndices[n] = nIdx - 1;
						}

						n += 1;
					}

					const bool b0 =
						(triangle.vIndices[0] >= 0 && triangle.vIndices[1] >= 0 && triangle.vIndices[2] >= 0) &&
						(triangle.tIndices[0] >= 0 && triangle.tIndices[1] >= 0 && triangle.tIndices[2] >= 0) &&
						(triangle.nIndices[0] >= 0 && triangle.nIndices[1] >= 0 && triangle.nIndices[2] >= 0);
					const bool b1 =
						(triangle.vIndices[0] < vertices.size() && triangle.vIndices[1] < vertices.size() && triangle.vIndices[2] < vertices.size()) &&
						(triangle.tIndices[0] < texcoors.size() && triangle.tIndices[1] < texcoors.size() && triangle.tIndices[2] < texcoors.size()) &&
						(triangle.nIndices[0] < vnormals.size() && triangle.nIndices[1] < vnormals.size() && triangle.nIndices[2] < vnormals.size());

					assert(n == 3);
					assert(b0 && b1);

					if (b0 && b1) {
						// note: this assumes face elements may not reference indices
						// ahead of the current {vertices, texcoors, vnormals}.size()
						// if this *is* allowed, the entire file must be parsed first
						float3 &v0 = vertices[triangle.vIndices[0]], &v1 = vertices[triangle.vIndices[1]], &v2 = vertices[triangle.vIndices[2]];
						float2 &t0 = texcoors[triangle.tIndices[0]], &t1 = texcoors[triangle.tIndices[1]], &t2 = texcoors[triangle.tIndices[2]];
						float3 &n0 = vnormals[triangle.nIndices[0]], &n1 = vnormals[triangle.nIndices[1]], &n2 = vnormals[triangle.nIndices[2]];

						const int
							idx0 = (piece->GetTriangleCount() * 3) + 0,
							idx1 = (piece->GetTriangleCount() * 3) + 1,
							idx2 = (piece->GetTriangleCount() * 3) + 2;

						// convert to piece-local vtn indices
						triangle.vIndices[0] = idx0; piece->AddVertex(v0             );
						triangle.nIndices[0] = idx0; piece->AddNormal(n0.ANormalize());
						triangle.tIndices[0] = idx0; piece->AddTxCoor(t0             );

						triangle.vIndices[1] = idx1; piece->AddVertex(v1             );
						triangle.nIndices[1] = idx1; piece->AddNormal(n1.ANormalize());
						triangle.tIndices[1] = idx1; piece->AddTxCoor(t1             );

						triangle.vIndices[2] = idx2; piece->AddVertex(v2             );
						triangle.nIndices[2] = idx2; piece->AddNormal(n2.ANormalize());
						triangle.tIndices[2] = idx2; piece->AddTxCoor(t2             );

						piece->AddTriangle(triangle);
					} else {
						LOG_L(L_WARNING, "Illegal face-element indices on line \"%s\" for model \"%s\"", line.c_str(), model->name.c_str());
					}
				} break;

				default: {
				} break;
			}

			lineStream.clear();
			lineStream.str("");
		}

		prevReadIdx = currReadIdx + 1;
	}


	const LuaTable& piecesTable = metaData.SubTable("pieces");
	const bool globalVertexOffsets = metaData.GetBool("globalvertexoffsets", false);
	const bool localPieceOffsets = metaData.GetBool("localpieceoffsets", false);

	if (BuildModelPieceTree(model, pieceMap, piecesTable, globalVertexOffsets, localPieceOffsets))
		return true;

	for (PieceMap::iterator it = pieceMap.begin(); it != pieceMap.end(); ++it)
		delete (it->second);

	throw content_error("[OBJParser] model " + model->name + " has no uniquely defined root-piece");
	return false;
}


bool COBJParser::BuildModelPieceTree(
	S3DModel* model,
	const PieceMap& pieceMap,
	const LuaTable& piecesTable,
	bool globalVertexOffsets,
	bool localPieceOffsets
) {
	std::vector<std::string> rootPieceNames;
	std::vector<int> rootPieceNumbers;

	piecesTable.GetKeys(rootPieceNames);
	piecesTable.GetKeys(rootPieceNumbers);

	SOBJPiece* rootPiece = nullptr;

	// there must be exactly one root-piece defined by name or number
	if ((rootPieceNames.size() + rootPieceNumbers.size()) != 1)
		return false;

	if (!rootPieceNames.empty()) {
		const std::string& rootPieceName = rootPieceNames[0];
		const LuaTable& rootPieceTable = piecesTable.SubTable(rootPieceName);
		const PieceMap::const_iterator rootPieceIt = pieceMap.find(rootPieceName);

		if (rootPieceIt != pieceMap.end()) {
			rootPiece = rootPieceIt->second;
			model->SetRootPiece(rootPiece);

			BuildModelPieceTreeRec(model, rootPiece, pieceMap, rootPieceTable, globalVertexOffsets, localPieceOffsets);
			return true;
		}
	}

	if (!rootPieceNumbers.empty()) {
		const LuaTable& rootPieceTable = piecesTable.SubTable(rootPieceNumbers[0]);
		const std::string& rootPieceName = rootPieceTable.GetString("name", "");
		const PieceMap::const_iterator rootPieceIt = pieceMap.find(rootPieceName);

		if (rootPieceIt != pieceMap.end()) {
			rootPiece = rootPieceIt->second;
			model->SetRootPiece(rootPiece);

			BuildModelPieceTreeRec(model, rootPiece, pieceMap, rootPieceTable, globalVertexOffsets, localPieceOffsets);
			return true;
		}
	}

	return false;
}

void COBJParser::BuildModelPieceTreeRec(
	S3DModel* model,
	SOBJPiece* piece,
	const PieceMap& pieceMap,
	const LuaTable& pieceTable,
	bool globalVertexOffsets,
	bool localPieceOffsets
) {
	const S3DModelPiece* parentPiece = piece->parent;

	// first read user-set extrema; trust that mins < maxs (for SMME)
	piece->mins = pieceTable.GetFloat3("mins", DEF_MIN_SIZE);
	piece->maxs = pieceTable.GetFloat3("maxs", DEF_MAX_SIZE);

	// always convert <offset> to local coordinates
	piece->offset = pieceTable.GetFloat3("offset", ZeroVector);
	piece->goffset = (localPieceOffsets)?
		(piece->offset + ((parentPiece != nullptr)? parentPiece->goffset: ZeroVector)):
		(piece->offset);
	piece->offset = (localPieceOffsets)?
		(piece->offset):
		(piece->offset - ((parentPiece != nullptr)? parentPiece->offset: ZeroVector));

	piece->SetVertexTangents();
	piece->SetMinMaxExtends(globalVertexOffsets);

	model->mins = float3::min(piece->goffset + piece->mins, model->mins);
	model->maxs = float3::max(piece->goffset + piece->maxs, model->maxs);

	piece->SetCollisionVolume(CollisionVolume("box", piece->maxs - piece->mins, (piece->maxs + piece->mins) * 0.5f));

	std::vector<int> childPieceNumbers;
	std::vector<std::string> childPieceNames;

	pieceTable.GetKeys(childPieceNumbers);
	pieceTable.GetKeys(childPieceNames);

	if (!childPieceNames.empty()) {
		for (std::vector<std::string>::const_iterator it = childPieceNames.begin(); it != childPieceNames.end(); ++it) {
			// NOTE: handle these better?
			if ((*it) == "offset" || (*it) == "name") {
				continue;
			}

			const std::string& childPieceName = *it;
			const LuaTable& childPieceTable = pieceTable.SubTable(childPieceName);

			PieceMap::const_iterator pieceIt = pieceMap.find(childPieceName);

			if (pieceIt == pieceMap.end()) {
				throw content_error("[OBJParser] meta-data piece named \"" + childPieceName + "\" not defined in model");
			} else {
				SOBJPiece* childPiece = pieceIt->second;

				assert(childPieceName == childPiece->name);

				childPiece->parent = piece;
				piece->children.push_back(childPiece);

				BuildModelPieceTreeRec(model, childPiece, pieceMap, childPieceTable, globalVertexOffsets, localPieceOffsets);
			}
		}
	}

	if (!childPieceNumbers.empty()) {
		for (std::vector<int>::const_iterator it = childPieceNumbers.begin(); it != childPieceNumbers.end(); ++it) {
			const LuaTable& childPieceTable = pieceTable.SubTable(*it);
			const std::string& childPieceName = childPieceTable.GetString("name", "");

			PieceMap::const_iterator pieceIt = pieceMap.find(childPieceName);

			if (pieceIt == pieceMap.end()) {
				throw content_error("[OBJParser] meta-data piece named \"" + childPieceName + "\" not defined in model");
			} else {
				SOBJPiece* childPiece = pieceIt->second;

				assert(childPieceName == childPiece->name);

				childPiece->parent = piece;
				piece->children.push_back(childPiece);

				BuildModelPieceTreeRec(model, childPiece, pieceMap, childPieceTable, globalVertexOffsets, localPieceOffsets);
			}
		}
	}
}






void SOBJPiece::UploadGeometryVBOs()
{
	// cannot use HasGeometryData because indices is still empty
	if (triangles.empty())
		return;

	indices.reserve(GetTriangleCount() * 3);

	// generate the index-list; only needed when using VBO's
	for (unsigned int i = 0; i < GetTriangleCount(); i++) {
		const SOBJTriangle& tri = GetTriangle(i);
		indices.push_back(tri.vIndices[0]);
		indices.push_back(tri.vIndices[1]);
		indices.push_back(tri.vIndices[2]);
	}

	vboPositions.Bind(GL_ARRAY_BUFFER);
	vboPositions.New(vertices.size() * sizeof(float3), GL_STATIC_DRAW, &vertices[0]);
	vboPositions.Unbind();

	vboNormals.Bind(GL_ARRAY_BUFFER);
	vboNormals.New(vnormals.size() * sizeof(float3), GL_STATIC_DRAW, &vnormals[0]);
	vboNormals.Unbind();

	vboTexcoords.Bind(GL_ARRAY_BUFFER);
	vboTexcoords.New(texcoors.size() * sizeof(float2), GL_STATIC_DRAW, &texcoors[0]);
	vboTexcoords.Unbind();

	vbosTangents.Bind(GL_ARRAY_BUFFER);
	vbosTangents.New(sTangents.size() * sizeof(float3), GL_STATIC_DRAW, &sTangents[0]);
	vbosTangents.Unbind();

	vbotTangents.Bind(GL_ARRAY_BUFFER);
	vbotTangents.New(tTangents.size() * sizeof(float3), GL_STATIC_DRAW, &tTangents[0]);
	vbotTangents.Unbind();

	vboIndices.Bind(GL_ELEMENT_ARRAY_BUFFER);
	vboIndices.New(indices.size() * sizeof(unsigned int), GL_STATIC_DRAW, &indices[0]);
	vboIndices.Unbind();

	// FIXME:
	//   assumes vIndices, nIndices and tIndices are identical in layout for all vertices
	//   (not a big problem because OBJ models must have a normal and texcoord per vertex)


	// NOTE: wasteful to keep these around, but still needed (eg. for Shatter())
	// vertices.clear();
	// indices.clear();
	// vnormals.clear();
	// texcoors.clear();
	sTangents.clear();
	tTangents.clear();
	triangles.clear();
}


void SOBJPiece::BindVertexAttribVBOs() const
{
	vbosTangents.Bind(GL_ARRAY_BUFFER);
		glClientActiveTexture(GL_TEXTURE5);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), vbosTangents.GetPtr());
	vbosTangents.Unbind();

	vbotTangents.Bind(GL_ARRAY_BUFFER);
		glClientActiveTexture(GL_TEXTURE6);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), vbotTangents.GetPtr());
	vbotTangents.Unbind();

	vboTexcoords.Bind(GL_ARRAY_BUFFER);
		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(float2), vboTexcoords.GetPtr());

		glClientActiveTexture(GL_TEXTURE1);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(float2), vboTexcoords.GetPtr());
	vboTexcoords.Unbind();

	vboPositions.Bind(GL_ARRAY_BUFFER);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(float3), vboPositions.GetPtr());
	vboPositions.Unbind();

	vboNormals.Bind(GL_ARRAY_BUFFER);
		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, sizeof(float3), vboNormals.GetPtr());
	vboNormals.Unbind();
}


void SOBJPiece::UnbindVertexAttribVBOs() const
{
	glClientActiveTexture(GL_TEXTURE6);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE5);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}


void SOBJPiece::DrawForList() const
{
	if (!HasGeometryData())
		return;

	BindVertexAttribVBOs();
	vboIndices.Bind(GL_ELEMENT_ARRAY_BUFFER);
		glDrawRangeElements(GL_TRIANGLES, 0, vertices.size() - 1, indices.size(), GL_UNSIGNED_INT, vboIndices.GetPtr());
	vboIndices.Unbind();
	UnbindVertexAttribVBOs();
}

void SOBJPiece::SetMinMaxExtends(bool globalVertexOffsets)
{
	// if no user-set extrema, calculate them
	const bool overrideMins = (mins == DEF_MIN_SIZE);
	const bool overrideMaxs = (maxs == DEF_MAX_SIZE);

	for (int i = GetVertexCount() - 1; i >= 0; i--) {
		float3 vertexGlobalPos;
		float3 vertexLocalPos;

		if (globalVertexOffsets) {
			// metadata indicates vertices are defined in model-space
			//
			// for converted S3O's, the piece offsets are defined wrt.
			// the parent piece, *not* wrt. the root piece (<goffset>
			// stores the concatenated transform)
			vertexGlobalPos = GetVertexPos(i);
			vertexLocalPos = vertexGlobalPos - goffset;
		} else {
			vertexLocalPos = GetVertexPos(i);
			vertexGlobalPos = vertexLocalPos + goffset;
		}

		// NOTE: the min- and max-extends of a piece are not calculated
		// recursively over its children since this makes little sense
		// for (per-piece) coldet purposes; the model extends do bound
		// all pieces
		if (overrideMins) { mins = float3::min(mins, vertexLocalPos); }
		if (overrideMaxs) { maxs = float3::max(maxs, vertexLocalPos); }

		// we want vertices in piece-space
		SetVertex(i, vertexLocalPos);
	}
}

void SOBJPiece::SetVertexTangents()
{
	// cannot use HasGeometryData because indices is still empty
	if (triangles.empty())
		return;

	sTangents.resize(GetVertexCount(), ZeroVector);
	tTangents.resize(GetVertexCount(), ZeroVector);

	// set the triangle-level S- and T-tangents
	for (int i = GetTriangleCount() - 1; i >= 0; i--) {
		const SOBJTriangle& tri = GetTriangle(i);

		const float3&
			p0 = GetVertexPos(tri.vIndices[0]),
			p1 = GetVertexPos(tri.vIndices[1]),
			p2 = GetVertexPos(tri.vIndices[2]);
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

		if (s == ZeroVector) { s = RgtVector; }
		if (t == ZeroVector) { t =  UpVector; }

		h = ((n.cross(s)).dot(t) < 0.0f)? -1: 1;
		s = (s - n * n.dot(s));
		s = s.SafeANormalize();
		t = (s.cross(n)) * h;
	}
}

