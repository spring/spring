/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cctype>
#include <stdexcept>

#include "S3OParser.h"
#include "s3o.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "System/Exceptions.h"
#include "System/SpringMath.h"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/byteorder.h"



void CS3OParser::Init() { numPoolPieces = 0; }
void CS3OParser::Kill() {
	LOG_L(L_INFO, "[S3OParser::%s] allocated %u pieces", __func__, numPoolPieces);

	// reuse piece innards when reloading
	// piecePool.clear();
	for (unsigned int i = 0; i < numPoolPieces; i++) {
		piecePool[i].Clear();
	}

	numPoolPieces = 0;
}

S3DModel CS3OParser::Load(const std::string& name)
{
	CFileHandler file(name);
	std::vector<uint8_t> fileBuf;

	if (!file.FileExists())
		throw content_error("[S3OParser] could not find model-file " + name);

	if (!file.IsBuffered()) {
		fileBuf.resize(file.FileSize(), 0);
		file.Read(fileBuf.data(), fileBuf.size());
	} else {
		fileBuf = std::move(file.GetBuffer());
	}

	if (fileBuf.size() < sizeof(S3OHeader))
		throw content_error("[S3OParser] corrupted header for model-file " + name);

	S3OHeader header;
	memcpy(&header, fileBuf.data(), sizeof(header));
	header.swap();

	S3DModel model;
		model.name = name;
		model.type = MODELTYPE_S3O;
		model.numPieces = 0;
		model.texs[0] = (header.texture1 == 0)? "" : (char*) &fileBuf[header.texture1];
		model.texs[1] = (header.texture2 == 0)? "" : (char*) &fileBuf[header.texture2];
		model.mins = DEF_MIN_SIZE;
		model.maxs = DEF_MAX_SIZE;

	textureHandlerS3O.PreloadTexture(&model);

	model.FlattenPieceTree(LoadPiece(&model, nullptr, fileBuf, header.rootPiece));

	// set after the extrema are known
	model.radius = (header.radius <= 0.01f)? model.CalcDrawRadius(): header.radius;
	model.height = (header.height <= 0.01f)? model.CalcDrawHeight(): header.height;
	model.relMidPos = float3(header.midx, header.midy, header.midz);

	return model;
}


SS3OPiece* CS3OParser::AllocPiece()
{
	std::lock_guard<spring::mutex> lock(poolMutex);

	// lazily reserve pool here instead of during Init
	// this way games using only one model-type do not
	// cause redundant allocation
	if (piecePool.empty())
		piecePool.resize(MAX_MODEL_OBJECTS * 16);

	if (numPoolPieces >= piecePool.size()) {
		throw std::bad_alloc();
		return nullptr;
	}

	return &piecePool[numPoolPieces++];
}

SS3OPiece* CS3OParser::LoadPiece(S3DModel* model, SS3OPiece* parent, std::vector<uint8_t>& buf, int offset)
{
	if ((offset + sizeof(Piece)) > buf.size())
		throw content_error("[S3OParser] corrupted piece for model-file " + model->name);

	model->numPieces++;

	// retrieve piece data
	Piece* fp = reinterpret_cast<Piece*>(&buf[offset]); fp->swap();
	Vertex* vertexList = reinterpret_cast<Vertex*>(&buf[fp->vertices]);

	const int* indexList = reinterpret_cast<int*>(&buf[fp->vertexTable]);
	const int* childList = reinterpret_cast<int*>(&buf[fp->children]);

	// create piece
	SS3OPiece* piece = AllocPiece();

	piece->offset.x = fp->xoffset;
	piece->offset.y = fp->yoffset;
	piece->offset.z = fp->zoffset;
	piece->primType = fp->primitiveType;
	piece->name = (char*) &buf[fp->name];
	piece->parent = parent;

	// retrieve vertices
	piece->SetVertexCount(fp->numVertices);
	for (int a = 0; a < fp->numVertices; ++a) {
		Vertex* v = vertexList++;
		v->swap();

		SS3OVertex sv;
		sv.pos = float3(v->xpos, v->ypos, v->zpos);
		sv.normal = float3(v->xnormal, v->ynormal, v->znormal);

		if (sv.normal.CheckNaNs()) {
			sv.normal.SafeANormalize();
		} else {
			sv.normal = ZeroVector;
		}

		sv.texCoords[0] = float2(v->texu, v->texv);
		sv.texCoords[1] = float2(v->texu, v->texv);
		sv.pieceIndex = model->numPieces - 1;

		piece->SetVertex(a, sv);
	}

	// retrieve draw indices
	piece->SetIndexCount(fp->vertexTableSize);
	for (int a = 0; a < fp->vertexTableSize; ++a) {
		piece->SetIndex(a, swabDWord(*(indexList++)));
	}

	// post-process the piece
	{
		piece->SetGlobalOffset(CMatrix44f::Identity());
		piece->Trianglize();
		piece->SetVertexTangents();
		piece->SetMinMaxExtends();

		model->mins = float3::min(piece->goffset + piece->mins, model->mins);
		model->maxs = float3::max(piece->goffset + piece->maxs, model->maxs);

		piece->SetCollisionVolume(CollisionVolume('b', 'z', piece->maxs - piece->mins, (piece->maxs + piece->mins) * 0.5f));
	}

	// load children pieces
	piece->children.reserve(fp->numchildren);

	for (int a = 0; a < fp->numchildren; ++a) {
		const int childOffset = swabDWord(*(childList++));

		piece->children.push_back(LoadPiece(model, piece, buf, childOffset));
	}

	return piece;
}






void SS3OPiece::SetMinMaxExtends()
{
	for (const SS3OVertex& v: vertices) {
		mins = float3::min(mins, v.pos);
		maxs = float3::max(maxs, v.pos);
	}
}


void SS3OPiece::Trianglize()
{
	switch (primType) {
		case S3O_PRIMTYPE_TRIANGLES: {
		} break;
		case S3O_PRIMTYPE_TRIANGLE_STRIP: {
			if (indices.size() < 3) {
				primType = S3O_PRIMTYPE_TRIANGLES;
				indices.clear();
				return;
			}

			decltype(indices) newIndices;
			newIndices.resize(indices.size() * 3); // each index (can) create a new triangle

			for (size_t i = 0; (i + 2) < indices.size(); ++i) {
				// indices can contain end-of-strip markers (-1U)
				if (indices[i + 0] == -1 || indices[i + 1] == -1 || indices[i + 2] == -1)
					continue;

				newIndices.push_back(indices[i + 0]);
				newIndices.push_back(indices[i + 1]);
				newIndices.push_back(indices[i + 2]);
			}

			primType = S3O_PRIMTYPE_TRIANGLES;
			indices.swap(newIndices);
		} break;
		case S3O_PRIMTYPE_QUADS: {
			if (indices.size() % 4 != 0) {
				primType = S3O_PRIMTYPE_TRIANGLES;
				indices.clear();
				return;
			}

			decltype(indices) newIndices;
			const size_t oldCount = indices.size();
			newIndices.resize(oldCount + oldCount / 2); // 4 indices become 6

			for (size_t i = 0, j = 0; i < indices.size(); i += 4) {
				newIndices[j++] = indices[i + 0];
				newIndices[j++] = indices[i + 1];
				newIndices[j++] = indices[i + 2];

				newIndices[j++] = indices[i + 0];
				newIndices[j++] = indices[i + 2];
				newIndices[j++] = indices[i + 3];
			}

			primType = S3O_PRIMTYPE_TRIANGLES;
			indices.swap(newIndices);
		} break;

		default: {
		} break;
	}
}


void SS3OPiece::SetVertexTangents()
{
	if (!HasGeometryData())
		return;

	unsigned int stride = 0;

	switch (primType) {
		case S3O_PRIMTYPE_TRIANGLES     : { stride = 3; } break;
		case S3O_PRIMTYPE_TRIANGLE_STRIP: { stride = 1; } break;
		case S3O_PRIMTYPE_QUADS         : {     return; } break;
	}

	// set the triangle-level S- and T-tangents
	// for triangle strips, the piece vertex _indices_ are defined
	// by the draw order of the vertices numbered <v, v + 1, v + 2>
	// for v in [0, n - 2]
	for (unsigned int i = 0, n = indices.size() - 2 * (stride == 1); i < n; i += stride) {
		const bool flipWinding = ((primType == S3O_PRIMTYPE_TRIANGLE_STRIP) && ((i & 1) == 1));

		const int v0idx = indices[i                      ];
		const int v1idx = indices[i + (flipWinding? 2: 1)];
		const int v2idx = indices[i + (flipWinding? 1: 2)];

		if (v1idx == -1 || v2idx == -1) {
			// not a valid triangle, skip
			// to start of next tri-strip
			i += 3; continue;
		}

		SS3OVertex& v0 = vertices[v0idx];
		SS3OVertex& v1 = vertices[v1idx];
		SS3OVertex& v2 = vertices[v2idx];

		const float3& p0 = v0.pos;
		const float3& p1 = v1.pos;
		const float3& p2 = v2.pos;

		const float2& tc0 = v0.texCoords[0];
		const float2& tc1 = v1.texCoords[0];
		const float2& tc2 = v2.texCoords[0];

		const float3 p10 = p1 - p0;
		const float3 p20 = p2 - p0;

		const float2 tc10 = tc1 - tc0;
		const float2 tc20 = tc2 - tc0;

		// if d is 0, texcoors are degenerate
		const float d = (tc10.x * tc20.y - tc20.x * tc10.y);
		const float r = (d > -0.0001f && d < 0.0001f)? 1.0f: 1.0f / d;

		// note: not necessarily orthogonal to each other
		// or to vertex normal, only to the triangle plane
		const float3 sdir = {tc20.y * p10.x - tc10.y * p20.x, tc20.y * p10.y - tc10.y * p20.y, tc20.y * p10.z - tc10.y * p20.z};
		const float3 tdir = {tc10.x * p20.x - tc20.x * p10.x, tc10.x * p20.y - tc20.x * p10.y, tc10.x * p20.z - tc20.x * p10.z};

		v0.sTangent += (sdir * r);
		v1.sTangent += (sdir * r);
		v2.sTangent += (sdir * r);

		v0.tTangent += (tdir * r);
		v1.tTangent += (tdir * r);
		v2.tTangent += (tdir * r);
	}

	// set the smoothed per-vertex tangents
	for (unsigned int i = 0, n = vertices.size(); i < n; i++) {
		float3& N = vertices[i].normal;
		float3& T = vertices[i].sTangent;
		float3& B = vertices[i].tTangent; // bi

		N.AssertNaNs();
		T.AssertNaNs();
		B.AssertNaNs();

		const float bitangentAngle = B.dot(N.cross(T)); // dot(B,B')
		const float handednessSign = Sign(bitangentAngle);

		T = (T - N * N.dot(T));
		B = (N.cross(T.SafeANormalize())) * handednessSign;
	}
}
