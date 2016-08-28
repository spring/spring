/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "3DOParser.h"

#include "Sim/Misc/CollisionVolume.h"
#include "System/Exceptions.h"
#include "System/Util.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Platform/byteorder.h"
#include "System/Sync/HsiehHash.h"

#include <cctype>
#include <boost/cstdint.hpp>


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


static void READ_3DOBJECT(C3DOParser::_3DObject& o, const std::vector<unsigned char>& fileBuf, int& curOffset)
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


static void READ_PRIMITIVE(C3DOParser::_Primitive& p, const std::vector<unsigned char>& fileBuf, int& curOffset)
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

C3DOParser::C3DOParser()
{
	CFileHandler file("unittextures/tatex/teamtex.txt");
	CSimpleParser parser(file);

	while (!parser.Eof()) {
		teamtex.insert(StringToLower(parser.GetCleanLine()));
	}
}


S3DModel* C3DOParser::Load(const std::string& name)
{
	CFileHandler file(name);
	std::vector<unsigned char> fileBuf;

	if (!file.FileExists()) {
		throw content_error("[3DOParser] could not find model-file " + name);
	}

	fileBuf.resize(file.FileSize(), 0);

	if (file.Read(&fileBuf[0], file.FileSize()) == 0) {
		throw content_error("[3DOParser] failed to read model-file " + name);
	}

	S3DModel* model = new S3DModel();
		model->name = name;
		model->type = MODELTYPE_3DO;
		model->textureType = 0;
		model->numPieces  = 0;
		model->mins = DEF_MIN_SIZE;
		model->maxs = DEF_MAX_SIZE;

	S3DOPiece* rootPiece = LoadPiece(model, 0, NULL, &model->numPieces, fileBuf);
	model->SetRootPiece(rootPiece);

	// set after the extrema are known
	model->radius = (model->maxs   - model->mins  ).Length() * 0.5f;
	model->height = (model->maxs.y - model->mins.y);
	model->relMidPos = (model->maxs + model->mins) * 0.5f;

	return model;
}


void C3DOParser::GetVertexes(_3DObject* o, S3DOPiece* object, const std::vector<unsigned char>& fileBuf)
{
	int curOffset = o->OffsetToVertexArray;
	object->vertexPos.resize(o->NumberOfVertices);

	for (int a = 0; a < o->NumberOfVertices; a++) {
		float3 v;
		READ_VERTEX(v, fileBuf, curOffset);
		v *= SCALE_FACTOR_3DO;
		v.z = -v.z;

		object->vertexPos[a] = v;
	}
}


bool C3DOParser::IsBasePlate(S3DOPiece* obj, S3DOPrimitive* face)
{
	if (!(face->primNormal.dot(-UpVector) > 0.99f))
		return false;

	if (face->indices.size() != 4)
		return false;

	const float3 s1 = obj->vertexPos[face->indices[0]] - obj->vertexPos[face->indices[1]];
	const float3 s2 = obj->vertexPos[face->indices[1]] - obj->vertexPos[face->indices[2]];

	if (s1.SqLength()<900 || s2.SqLength()<900)
		return false;

	for(int vi: face->indices) {
		if (obj->vertexPos[vi].y > 0) {
			return false;
		}
	}

	return true;
}


C3DOTextureHandler::UnitTexture* C3DOParser::GetTexture(S3DOPiece* obj, _Primitive* p, const std::vector<unsigned char>& fileBuf) const
{
	std::string texName;
	if (p->OffsetToTextureName != 0) {
		int unused;
		texName = GET_TEXT(p->OffsetToTextureName, fileBuf, unused);
		StringToLowerInPlace(texName);

		if (teamtex.find(texName) == teamtex.end()) {
			texName += "00";
		}
	} else {
		texName = "ta_color" + IntToString(p->PaletteEntry, "%i");
	}

	auto tex = texturehandler3DO->Get3DOTexture(texName);
	if (tex != nullptr)
		return tex;

	LOG_L(L_WARNING, "[%s] unknown 3DO texture \"%s\" for piece \"%s\"",
			__FUNCTION__, texName.c_str(), obj->name.c_str());

	// assign a dummy texture (the entire atlas)
	return texturehandler3DO->Get3DOTexture("___dummy___");
}


void C3DOParser::GetPrimitives(S3DOPiece* obj, int pos, int num, int excludePrim, const std::vector<unsigned char>& fileBuf)
{
	std::map<int,int> prevHashes;

	for (int a=0; a<num; a++) {
		if (a == excludePrim) {
			continue;
		}

		_Primitive p;
		int curOffset = pos + a * sizeof(_Primitive);
		READ_PRIMITIVE(p, fileBuf, curOffset);

		if (p.NumberOfVertexIndexes < 3)
			continue;

		S3DOPrimitive sp;
		sp.indices.resize(p.NumberOfVertexIndexes);
		sp.vnormals.resize(p.NumberOfVertexIndexes);

		// load vertex indices list
		curOffset = p.OffsetToVertexIndexArray;
		for (int b=0; b<p.NumberOfVertexIndexes; b++) {
			boost::uint16_t w;
			STREAM_READ(&w,2, fileBuf, curOffset);
			swabWordInPlace(w);
			sp.indices[b] = w;
		}

		// find texture
		sp.texture = GetTexture(obj, &p, fileBuf);

		// set the primitive-normal
		const float3 v0v1 = (obj->vertexPos[sp.indices[1]] - obj->vertexPos[sp.indices[0]]);
		const float3 v0v2 = (obj->vertexPos[sp.indices[2]] - obj->vertexPos[sp.indices[0]]);
		sp.primNormal = (-v0v1.cross(v0v2)).SafeANormalize();

		// some 3dos got multiple baseplates a.k.a. `selection primitive`
		// it's not meant to be rendered, so hide it
		if (IsBasePlate(obj, &sp))
			continue;

		// 3do has often duplicated faces (with equal geometry)
		// with different textures (e.g. for animations and other effects)
		// we don't support those, only render the last one
		std::vector<int> orderVert = sp.indices;
		std::sort(orderVert.begin(), orderVert.end());
		const int vertHash = HsiehHash(&orderVert[0], orderVert.size() * sizeof(orderVert[0]), 0x123456);

		auto phi = prevHashes.find(vertHash);
		if (phi != prevHashes.end()) {
			obj->prims[phi->second] = sp;
			continue;
		}
		prevHashes[vertHash] = obj->prims.size();
		obj->prims.push_back(sp);
	}
}


S3DOPiece* C3DOParser::LoadPiece(S3DModel* model, int pos, S3DOPiece* parent, int* numobj, const std::vector<unsigned char>& fileBuf)
{
	(*numobj)++;

	_3DObject me;
	int curOffset = pos;
	READ_3DOBJECT(me, fileBuf, curOffset);

	std::string s = GET_TEXT(me.OffsetToObjectName, fileBuf, curOffset);
	StringToLowerInPlace(s);

	S3DOPiece* piece = new S3DOPiece();
		piece->name = s;
		piece->parent = parent;
		piece->offset.x =  me.XFromParent * SCALE_FACTOR_3DO;
		piece->offset.y =  me.YFromParent * SCALE_FACTOR_3DO;
		piece->offset.z = -me.ZFromParent * SCALE_FACTOR_3DO;
		piece->goffset = piece->offset + ((parent != NULL)? parent->goffset: ZeroVector);

	GetVertexes(&me, piece, fileBuf);
	GetPrimitives(piece, me.OffsetToPrimitiveArray, me.NumberOfPrimitives, ((pos == 0)? me.SelectionPrimitive: -1), fileBuf);
	piece->CalcNormals();
	piece->SetMinMaxExtends();

	piece->emitPos = ZeroVector;
	piece->emitDir = FwdVector;
	if (piece->vertexPos.size() >= 2) {
		piece->emitPos = piece->vertexPos[0];
		piece->emitDir = piece->vertexPos[1] - piece->vertexPos[0];
	} else 	if (piece->vertexPos.size() == 1) {
		piece->emitDir = piece->vertexPos[0];
	}

	model->mins = float3::min(piece->goffset + piece->mins, model->mins);
	model->maxs = float3::max(piece->goffset + piece->maxs, model->maxs);

	piece->SetCollisionVolume(CollisionVolume("box", piece->maxs - piece->mins, (piece->maxs + piece->mins) * 0.5f));

	if (me.OffsetToChildObject > 0) {
		piece->children.push_back(LoadPiece(model, me.OffsetToChildObject, piece, numobj, fileBuf));
	}

	if (me.OffsetToSiblingObject > 0) {
		parent->children.push_back(LoadPiece(model, me.OffsetToSiblingObject, parent, numobj, fileBuf));
	}

	return piece;
}


void S3DOPiece::UploadGeometryVBOs()
{
	// cannot use HasGeometryData because vboIndices is still empty
	if (prims.empty())
		return;

	std::vector<unsigned int>& indices = vertexIndices;
	std::vector<S3DOVertex>& vertices = vertexAttribs;

	// assume all faces are quads
	indices.reserve(prims.size() * 6);
	vertices.reserve(prims.size() * 4);

	// trianglize all input
	for (const S3DOPrimitive& ps: prims) {
		C3DOTextureHandler::UnitTexture* tex = ps.texture;

		if (ps.indices.size() == 4) {
			// quad
			indices.push_back(vertices.size() + 0);
			indices.push_back(vertices.size() + 1);
			indices.push_back(vertices.size() + 2);
			indices.push_back(vertices.size() + 0);
			indices.push_back(vertices.size() + 2);
			indices.push_back(vertices.size() + 3);
			vertices.emplace_back(vertexPos[ps.indices[0]], ps.vnormals[0], float2(tex->xstart, tex->ystart));
			vertices.emplace_back(vertexPos[ps.indices[1]], ps.vnormals[1], float2(tex->xend,   tex->ystart));
			vertices.emplace_back(vertexPos[ps.indices[2]], ps.vnormals[2], float2(tex->xend,   tex->yend));
			vertices.emplace_back(vertexPos[ps.indices[3]], ps.vnormals[3], float2(tex->xstart, tex->yend));
		} else if (ps.indices.size() == 3) {
			// triangle
			indices.push_back(vertices.size() + 0);
			indices.push_back(vertices.size() + 1);
			indices.push_back(vertices.size() + 2);
			vertices.emplace_back(vertexPos[ps.indices[0]], ps.vnormals[0], float2(tex->xstart, tex->ystart));
			vertices.emplace_back(vertexPos[ps.indices[1]], ps.vnormals[1], float2(tex->xend,   tex->ystart));
			vertices.emplace_back(vertexPos[ps.indices[2]], ps.vnormals[2], float2(tex->xend,   tex->yend));
		} else if (ps.indices.size() >= 3) {
			// fan
			for (int i = 2; i < ps.indices.size(); ++i) {
				indices.push_back(vertices.size() + 0);
				indices.push_back(vertices.size() + i - 1);
				indices.push_back(vertices.size() + i - 0);
			}
			for (int i = 0; i < ps.indices.size(); ++i) {
				vertices.emplace_back(vertexPos[ps.indices[i]], ps.vnormals[i], float2(tex->xstart, tex->ystart));
			}
		}
	}


	//FIXME share 1 VBO for ALL models
	vboAttributes.Bind(GL_ARRAY_BUFFER);
	vboAttributes.New(vertices.size() * sizeof(S3DOVertex), GL_STATIC_DRAW, &vertices[0]);
	vboAttributes.Unbind();

	vboIndices.Bind(GL_ELEMENT_ARRAY_BUFFER);
	vboIndices.New(indices.size() * sizeof(unsigned), GL_STATIC_DRAW, &indices[0]);
	vboIndices.Unbind();

	// NOTE: wasteful to keep these around, but still needed (eg. for Shatter())
	// vertices.clear();
	// indices.clear();
}


void S3DOPiece::BindVertexAttribVBOs() const
{
	vboAttributes.Bind(GL_ARRAY_BUFFER);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(S3DOVertex), vboAttributes.GetPtr(offsetof(S3DOVertex, pos)));

		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, sizeof(S3DOVertex), vboAttributes.GetPtr(offsetof(S3DOVertex, normal)));

		glClientActiveTexture(GL_TEXTURE1);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(S3DOVertex), vboAttributes.GetPtr(offsetof(S3DOVertex, texCoord)));

		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(S3DOVertex), vboAttributes.GetPtr(offsetof(S3DOVertex, texCoord)));
	vboAttributes.Unbind();
}


void S3DOPiece::UnbindVertexAttribVBOs() const
{
	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}


void S3DOPiece::DrawForList() const
{
	if (!HasGeometryData())
		return;

	BindVertexAttribVBOs();
	vboIndices.Bind(GL_ELEMENT_ARRAY_BUFFER);
	glDrawRangeElements(GL_TRIANGLES, 0, (vboAttributes.GetSize() - 1) / sizeof(S3DOVertex), vboIndices.GetSize() / sizeof(unsigned), GL_UNSIGNED_INT, vboIndices.GetPtr());
	vboIndices.Unbind();
	UnbindVertexAttribVBOs();
}


void S3DOPiece::CalcNormals()
{
	// generate for each vertex a list of faces that share it
	std::vector<std::vector<int>> vertexToFaceIdx;
	vertexToFaceIdx.resize(vertexPos.size());
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
	for (float3 vp: vertexPos) {
		mins = float3::min(mins, vp);
		maxs = float3::max(maxs, vp);
	}
}
