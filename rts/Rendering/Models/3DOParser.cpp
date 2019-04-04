/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cctype>
#include <cinttypes>

#include "3DOParser.h"

#include "Sim/Misc/CollisionVolume.h"
#include "System/Exceptions.h"
#include "System/UnorderedMap.hpp"
#include "System/StringUtil.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Platform/byteorder.h"
#include "System/Sync/HsiehHash.h"



#define SCALE_FACTOR_3DO (1.0f / 65536.0f)


//////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////


static void STREAM_READ(void* buf, int length, const std::vector<unsigned char>& fileBuf, int& curOffset)
{
	memcpy(buf, &fileBuf[curOffset], length);
	curOffset += length;
}


static std::string GET_TEXT(int pos, const std::vector<unsigned char>& fileBuf, int& curOffset)
{
	curOffset = pos;
	std::string s;
	s.reserve(16);
	do {
		s.push_back(0);
		STREAM_READ(&s.back(), 1, fileBuf, curOffset);
	} while (s.back() != 0);
	s.pop_back(); // pop the \0
	return s;
}


static void READ_3DOBJECT(TA3DO::_3DObject& o, const std::vector<unsigned char>& fileBuf, int& curOffset)
{
	unsigned int __tmp;
	unsigned short __isize = sizeof(unsigned int);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	o.VersionSignature = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	o.NumberOfVertices = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	o.NumberOfPrimitives = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	o.SelectionPrimitive = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	o.XFromParent = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	o.YFromParent = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	o.ZFromParent = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	o.OffsetToObjectName = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	o.Always_0 = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	o.OffsetToVertexArray = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	o.OffsetToPrimitiveArray = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	o.OffsetToSiblingObject = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	o.OffsetToChildObject = (int)swabDWord(__tmp);
}


static void READ_VERTEX(float3& v, const std::vector<unsigned char>& fileBuf, int& curOffset)
{
	unsigned int __tmp;
	unsigned short __isize = sizeof(unsigned int);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	v.x = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	v.y = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	v.z = (int)swabDWord(__tmp);
}


static void READ_PRIMITIVE(TA3DO::_Primitive& p, const std::vector<unsigned char>& fileBuf, int& curOffset)
{
	unsigned int __tmp;
	unsigned short __isize = sizeof(unsigned int);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	p.PaletteEntry = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	p.NumberOfVertexIndexes = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	p.Always_0 = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	p.OffsetToVertexIndexArray = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	p.OffsetToTextureName = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	p.Unknown_1 = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	p.Unknown_2 = (int)swabDWord(__tmp);
	STREAM_READ(&__tmp,__isize, fileBuf, curOffset);
	p.Unknown_3 = (int)swabDWord(__tmp);
}





//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void C3DOParser::Init()
{
	CFileHandler file("unittextures/tatex/teamtex.txt");
	CSimpleParser parser(file);

	while (!parser.Eof()) {
		teamTextures.insert(StringToLower(parser.GetCleanLine()));
	}

	numPoolPieces = 0;
}

void C3DOParser::Kill()
{
	teamTextures.clear();
	LOG_L(L_INFO, "[3DOParser::%s] allocated %u pieces", __func__, numPoolPieces);

	// reuse piece innards when reloading
	// piecePool.clear();
	for (unsigned int i = 0; i < numPoolPieces; i++) {
		piecePool[i].Clear();
	}

	numPoolPieces = 0;
}


S3DModel C3DOParser::Load(const std::string& name)
{
	CFileHandler file(name);
	std::vector<uint8_t> fileBuf;

	if (!file.FileExists())
		throw content_error("[3DOParser] could not find model-file " + name);

	if (!file.IsBuffered()) {
		fileBuf.resize(file.FileSize(), 0);

		if (file.Read(fileBuf.data(), fileBuf.size()) == 0)
			throw content_error("[3DOParser] failed to read model-file " + name);
	} else {
		fileBuf = std::move(file.GetBuffer());
	}

	S3DModel model;
		model.name = name;
		model.type = MODELTYPE_3DO;
		model.textureType = 0;
		model.numPieces   = 0;
		model.mins = DEF_MIN_SIZE;
		model.maxs = DEF_MAX_SIZE;

	model.FlattenPieceTree(LoadPiece(&model, nullptr, fileBuf, 0));

	// set after the extrema are known
	model.radius = model.CalcDrawRadius();
	model.height = model.CalcDrawHeight();
	model.relMidPos = model.CalcDrawMidPos();

	return model;
}


void S3DOPiece::GetVertices(const TA3DO::_3DObject* o, const std::vector<unsigned char>& fileBuf)
{
	int curOffset = o->OffsetToVertexArray;

	verts.clear();
	verts.resize(o->NumberOfVertices);

	for (int a = 0; a < o->NumberOfVertices; a++) {
		float3 v;
		READ_VERTEX(v, fileBuf, curOffset);
		v *= SCALE_FACTOR_3DO;
		v.z = -v.z;

		verts[a] = v;
	}
}


bool S3DOPiece::IsBasePlate(const S3DOPrimitive* face) const
{
	if (!(face->primNormal.dot(-UpVector) > 0.99f))
		return false;

	if (face->indices.size() != 4)
		return false;

	const float3 s1 = verts[face->indices[0]] - verts[face->indices[1]];
	const float3 s2 = verts[face->indices[1]] - verts[face->indices[2]];

	if (s1.SqLength() < 900.0f || s2.SqLength() < 900.0f)
		return false;

	for (int vi: face->indices) {
		if (verts[vi].y > 0.0f)
			return false;
	}

	return true;
}


C3DOTextureHandler::UnitTexture* S3DOPiece::GetTexture(
	const TA3DO::_Primitive* p,
	const std::vector<unsigned char>& fileBuf,
	const spring::unordered_set<std::string>& teamTextures
) const {
	std::string texName;

	if (p->OffsetToTextureName != 0) {
		int unused;
		texName = std::move(StringToLower(GET_TEXT(p->OffsetToTextureName, fileBuf, unused)));

		if (teamTextures.find(texName) == teamTextures.end()) {
			texName += "00";
		}
	} else {
		texName = "ta_color" + IntToString(p->PaletteEntry, "%i");
	}

	auto tex = textureHandler3DO.Get3DOTexture(texName);
	if (tex != nullptr)
		return tex;

	LOG_L(L_WARNING, "[%s] unknown 3DO texture \"%s\" for piece \"%s\"", __func__, texName.c_str(), name.c_str());

	// assign a dummy texture (the entire atlas)
	return textureHandler3DO.Get3DOTexture("___dummy___");
}


void S3DOPiece::GetPrimitives(
	const S3DModel* model,
	int pos,
	int num,
	int excludePrim,
	const std::vector<unsigned char>& fileBuf,
	const spring::unordered_set<std::string>& teamTextures
) {
	spring::unordered_map<int, int> prevHashes;
	std::vector<int> sortedVerts;

	for (int a = 0; a < num; a++) {
		if (a == excludePrim)
			continue;

		TA3DO::_Primitive p;
		int curOffset = pos + a * sizeof(p);
		READ_PRIMITIVE(p, fileBuf, curOffset);

		if (p.NumberOfVertexIndexes < 3)
			continue;

		S3DOPrimitive sp;
		sp.indices.resize(p.NumberOfVertexIndexes);
		sp.vnormals.resize(p.NumberOfVertexIndexes);

		// load vertex indices list
		curOffset = p.OffsetToVertexIndexArray;

		for (int b = 0; b < p.NumberOfVertexIndexes; b++) {
			std::uint16_t w;
			STREAM_READ(&w, 2, fileBuf, curOffset);
			swabWordInPlace(w);
			sp.indices[b] = w;
		}

		// find texture
		sp.texture = GetTexture(&p, fileBuf, teamTextures);

		// set the primitive-normal
		const float3 v0v1 = (verts[sp.indices[1]] - verts[sp.indices[0]]);
		const float3 v0v2 = (verts[sp.indices[2]] - verts[sp.indices[0]]);
		sp.primNormal = (-v0v1.cross(v0v2)).SafeANormalize();

		sp.pieceIndex = model->numPieces - 1;

		// some 3DO's have multiple baseplates (selection primitives)
		// which are not meant to be rendered, so hide them
		if (IsBasePlate(&sp))
			continue;

		// 3do has often duplicated faces (with equal geometry)
		// with different textures (e.g. for animations and other effects)
		// we don't support those, only render the last one
		sortedVerts.clear();
		sortedVerts.resize(sp.indices.size());

		std::copy(sp.indices.begin(), sp.indices.end(), sortedVerts.begin());
		std::sort(sortedVerts.begin(), sortedVerts.end());

		const int vertHash = HsiehHash(&sortedVerts[0], sortedVerts.size() * sizeof(sortedVerts[0]), 0x123456);

		const auto phi = prevHashes.find(vertHash);
		if (phi != prevHashes.end()) {
			prims[phi->second] = sp;
			continue;
		}

		prevHashes[vertHash] = prims.size();
		prims.push_back(sp);
	}
}


S3DOPiece* C3DOParser::AllocPiece()
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

S3DOPiece* C3DOParser::LoadPiece(S3DModel* model, S3DOPiece* parent, const std::vector<uint8_t>& buf, int pos)
{
	if ((pos + sizeof(TA3DO::_3DObject)) > buf.size())
		throw content_error("[3DOParser] corrupted piece for model-file " + model->name);

	model->numPieces++;

	TA3DO::_3DObject me;
	int curOffset = pos;
	READ_3DOBJECT(me, buf, curOffset);

	S3DOPiece* piece = AllocPiece();

	piece->name = std::move(StringToLower(GET_TEXT(me.OffsetToObjectName, buf, curOffset)));
	piece->parent = parent;
	piece->offset.x =  me.XFromParent * SCALE_FACTOR_3DO;
	piece->offset.y =  me.YFromParent * SCALE_FACTOR_3DO;
	piece->offset.z = -me.ZFromParent * SCALE_FACTOR_3DO;

	piece->SetGlobalOffset(CMatrix44f::Identity());
	piece->GetVertices(&me, buf);
	piece->GetPrimitives(model, me.OffsetToPrimitiveArray, me.NumberOfPrimitives, ((pos == 0)? me.SelectionPrimitive: -1), buf, teamTextures);

	piece->CalcNormals();
	piece->SetMinMaxExtends();
	piece->GenTriangleGeometry();

	switch (piece->verts.size()) {
		case 0: { piece->emitDir =    FwdVector   ; } break;
		case 1: { piece->emitDir = piece->verts[0]; } break;
		default: {
			piece->emitPos = piece->verts[0];
			piece->emitDir = piece->verts[1] - piece->verts[0];
		} break;
	}

	model->mins = float3::min(piece->goffset + piece->mins, model->mins);
	model->maxs = float3::max(piece->goffset + piece->maxs, model->maxs);

	piece->SetCollisionVolume(CollisionVolume('b', 'z', piece->maxs - piece->mins, (piece->maxs + piece->mins) * 0.5f));

	if (me.OffsetToChildObject > 0)
		piece->children.push_back(LoadPiece(model, piece, buf, me.OffsetToChildObject));

	if (me.OffsetToSiblingObject > 0)
		parent->children.push_back(LoadPiece(model, parent, buf, me.OffsetToSiblingObject));

	return piece;
}


void S3DOPiece::GenTriangleGeometry()
{
	std::vector<unsigned int>& indices = vertexIndices;
	std::vector<S3DOVertex>& vertices = vertexAttribs;

	// assume all faces are quads
	indices.reserve(prims.size() * 6);
	vertices.reserve(prims.size() * 4);

	// trianglize all input
	for (const S3DOPrimitive& ps: prims) {
		C3DOTextureHandler::UnitTexture* tex = ps.texture;
		S3DOVertex vtx;

		switch (ps.indices.size()) {
			case 0: {} break;
			case 1: {} break;
			case 2: {} break;
			case 4: {
				// quad
				indices.push_back(vertices.size() + 0);
				indices.push_back(vertices.size() + 1);
				indices.push_back(vertices.size() + 2);
				indices.push_back(vertices.size() + 0);
				indices.push_back(vertices.size() + 2);
				indices.push_back(vertices.size() + 3);

				vertices.emplace_back(verts[ps.indices[0]], ps.vnormals[0], float3{}, float3{}, float2(tex->xstart, tex->ystart), float2{}, ps.pieceIndex);
				vertices.emplace_back(verts[ps.indices[1]], ps.vnormals[1], float3{}, float3{}, float2(tex->xend,   tex->ystart), float2{}, ps.pieceIndex);
				vertices.emplace_back(verts[ps.indices[2]], ps.vnormals[2], float3{}, float3{}, float2(tex->xend,   tex->yend  ), float2{}, ps.pieceIndex);
				vertices.emplace_back(verts[ps.indices[3]], ps.vnormals[3], float3{}, float3{}, float2(tex->xstart, tex->yend  ), float2{}, ps.pieceIndex);
			} break;
			case 3: {
				// triangle
				indices.push_back(vertices.size() + 0);
				indices.push_back(vertices.size() + 1);
				indices.push_back(vertices.size() + 2);

				vertices.emplace_back(verts[ps.indices[0]], ps.vnormals[0], float3{}, float3{}, float2(tex->xstart, tex->ystart), float2{}, ps.pieceIndex);
				vertices.emplace_back(verts[ps.indices[1]], ps.vnormals[1], float3{}, float3{}, float2(tex->xend,   tex->ystart), float2{}, ps.pieceIndex);
				vertices.emplace_back(verts[ps.indices[2]], ps.vnormals[2], float3{}, float3{}, float2(tex->xend,   tex->yend  ), float2{}, ps.pieceIndex);
			} break;
			default: {
				// fan
				for (int i = 2; i < ps.indices.size(); ++i) {
					indices.push_back(vertices.size() + 0);
					indices.push_back(vertices.size() + i - 1);
					indices.push_back(vertices.size() + i - 0);
				}
				for (int i = 0; i < ps.indices.size(); ++i) {
					vertices.emplace_back(verts[ps.indices[i]], ps.vnormals[i], float3{}, float3{}, float2(tex->xstart, tex->ystart), float2{}, ps.pieceIndex);
				}
			} break;
		}
	}
}



void S3DOPiece::CalcNormals()
{
	// generate for each vertex a list of faces that share it
	std::vector<std::vector<int>> vertexToFaceIdx;
	vertexToFaceIdx.resize(verts.size());
	for (int i = 0; i < prims.size(); ++i) {
		for (auto& idx: prims[i].indices) {
			vertexToFaceIdx[idx].push_back(i);
		}
	}

	// and now smooth/average the normals of those faces
	for (S3DOPrimitive& curFace: prims) {
		const float3& curFaceNormal = curFace.primNormal;

		for (int a = 0; a < curFace.indices.size(); ++a) {
			const int vertIdx = curFace.indices[a];
			const auto& faceIndices = vertexToFaceIdx[vertIdx];

			// visit all primitives shared by this vertex
			// and smooth the face normals, Gouraud-style
			float3 smoothedNormal;
			for (int fidx: faceIndices) {
				const float3& faceNormal = prims[fidx].primNormal;

				// consider two primitives part of the same surface iff
				// angle between their normals is less than ~63 degrees
				if (curFaceNormal.dot(faceNormal) > 0.45f) {
					smoothedNormal += faceNormal;
				}
			}

			// now make the normal for vertex <a> equal the smoothed normal
			curFace.vnormals[a] = smoothedNormal.SafeANormalize();
		}
	}
}


void S3DOPiece::SetMinMaxExtends()
{
	for (const float3 vp: verts) {
		mins = float3::min(mins, vp);
		maxs = float3::max(maxs, vp);
	}
}

