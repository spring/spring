/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <cctype>
#include <stdexcept>

#include "S3OParser.h"
#include "s3o.h"
#include "Game/GlobalUnsynced.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GlobalRendering.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "System/Exceptions.h"
#include "System/Util.h"
#include "System/Vec2.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/FileHandler.h"
#include "System/Platform/byteorder.h"

static const float3 DEF_MIN_SIZE( 10000.0f,  10000.0f,  10000.0f);
static const float3 DEF_MAX_SIZE(-10000.0f, -10000.0f, -10000.0f);

S3DModel* CS3OParser::Load(const std::string& name)
{
	CFileHandler file(name);
	if (!file.FileExists()) {
		throw content_error("[S3OParser] could not find model-file " + name);
	}

	unsigned char* fileBuf = new unsigned char[file.FileSize()];
	file.Read(fileBuf, file.FileSize());
	S3OHeader header;
	memcpy(&header, fileBuf, sizeof(header));
	header.swap();

	S3DModel* model = new S3DModel;
		model->name = name;
		model->type = MODELTYPE_S3O;
		model->numPieces = 0;
		model->tex1 = (char*) &fileBuf[header.texture1];
		model->tex2 = (char*) &fileBuf[header.texture2];
		model->mins = DEF_MIN_SIZE;
		model->maxs = DEF_MAX_SIZE;
	texturehandlerS3O->LoadS3OTexture(model);

	SS3OPiece* rootPiece = LoadPiece(model, NULL, fileBuf, header.rootPiece);

	model->SetRootPiece(rootPiece);
	model->radius = (header.radius <= 0.01f)? (model->maxs.y - model->mins.y): header.radius;
	model->height = (header.height <= 0.01f)? (model->radius + model->radius): header.height;
	model->drawRadius = std::max(std::fabs(model->maxs), std::fabs(model->mins)).Length();
	
	model->relMidPos = float3(header.midx, header.midy, header.midz);
	model->relMidPos.y = std::max(model->relMidPos.y, 1.0f); // ?

	delete[] fileBuf;
	return model;
}

SS3OPiece* CS3OParser::LoadPiece(S3DModel* model, SS3OPiece* parent, unsigned char* buf, int offset)
{
	model->numPieces++;

	Piece* fp = (Piece*)&buf[offset];
	fp->swap(); // Does it matter we mess with the original buffer here? Don't hope so.

	SS3OPiece* piece = new SS3OPiece();
		piece->type = MODELTYPE_S3O;
		piece->mins = DEF_MIN_SIZE;
		piece->maxs = DEF_MAX_SIZE;
		piece->offset.x = fp->xoffset;
		piece->offset.y = fp->yoffset;
		piece->offset.z = fp->zoffset;
		piece->primitiveType = fp->primitiveType;
		piece->name = (char*) &buf[fp->name];
		piece->parent = parent;

	piece->SetVertexCount(fp->numVertices);
	piece->SetNormalCount(fp->numVertices);
	piece->SetTxCoorCount(fp->numVertices);
	piece->SetVertexDrawIndexCount(fp->vertexTableSize);

	// retrieve each vertex
	int vertexOffset = fp->vertices;

	for (int a = 0; a < fp->numVertices; ++a) {
		Vertex* v = reinterpret_cast<Vertex*>(&buf[vertexOffset]);
			v->swap();
		SS3OVertex* sv = reinterpret_cast<SS3OVertex*>(&buf[vertexOffset]);
			sv->normal.SafeANormalize();

		piece->SetVertex(a, sv->pos);
		piece->SetNormal(a, sv->normal);
		piece->SetTxCoor(a, float2(sv->textureX, sv->textureY));
		vertexOffset += sizeof(Vertex);
	}


	// retrieve the draw order for the vertices
	int vertexTableOffset = fp->vertexTable;

	for (int a = 0; a < fp->vertexTableSize; ++a) {
		const int vertexDrawIdx = swabDWord(*(int*) &buf[vertexTableOffset]);

		piece->SetVertexDrawIndex(a, vertexDrawIdx);
		vertexTableOffset += sizeof(int);
	}


	piece->isEmpty = (piece->GetVertexDrawIndexCount() == 0);
	piece->goffset = piece->offset + ((parent != NULL)? parent->goffset: ZeroVector);

	piece->SetVertexTangents();
	piece->SetMinMaxExtends();

	model->mins.x = std::min(piece->mins.x, model->mins.x);
	model->mins.y = std::min(piece->mins.y, model->mins.y);
	model->mins.z = std::min(piece->mins.z, model->mins.z);
	model->maxs.x = std::max(piece->maxs.x, model->maxs.x);
	model->maxs.y = std::max(piece->maxs.y, model->maxs.y);
	model->maxs.z = std::max(piece->maxs.z, model->maxs.z);

	const float3 cvScales = piece->maxs - piece->mins;
	const float3 cvOffset =
		(piece->maxs - piece->goffset) +
		(piece->mins - piece->goffset);

	piece->SetCollisionVolume(new CollisionVolume("box", cvScales, cvOffset * 0.5f));

	int childTableOffset = fp->children;

	for (int a = 0; a < fp->numchildren; ++a) {
		int childOffset = swabDWord(*(int*) &buf[childTableOffset]);

		SS3OPiece* childPiece = LoadPiece(model, piece, buf, childOffset);
		piece->children.push_back(childPiece);

		childTableOffset += sizeof(int);
	}

	return piece;
}






void SS3OPiece::UploadGeometryVBOs()
{
	if (isEmpty)
		return;

	// glBufferData reads beyond last element since either pos, normal or textureX will have a non-zero offset in SS30Vertex.
	vertices.reserve(vertices.size() + 1);
	sTangents.reserve(sTangents.size() + 1);
	tTangents.reserve(tTangents.size() + 1);

	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[VBO_VERTICES]);
	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float3), &(vertices[0].x), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[VBO_VNORMALS]);
	glBufferData(GL_ARRAY_BUFFER, vnormals.size() * sizeof(float3), &(vnormals[0].x), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[VBO_STANGENTS]);
	glBufferData(GL_ARRAY_BUFFER, sTangents.size() * sizeof(float3), &(sTangents[0].x), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[VBO_TTANGENTS]);
	glBufferData(GL_ARRAY_BUFFER, tTangents.size() * sizeof(float3), &(tTangents[0].x), GL_STATIC_DRAW);

	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[VBO_VTEXCOORS]);
	glBufferData(GL_ARRAY_BUFFER, texcoors.size() * sizeof(float2), &(texcoors[0].x), GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIDs[VBO_VINDICES]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vertexDrawIndices.size() * sizeof(unsigned int), &vertexDrawIndices[0], GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	// NOTE: wasteful to keep these around, but still needed (eg. for Shatter())
	// vertices.clear();
	// vnormals.clear();
	// texcoors.clear();
	// vertexDrawIndices.clear();
	sTangents.clear();
	tTangents.clear();
}

void SS3OPiece::DrawForList() const
{
	if (isEmpty)
		return;

	// pass the tangents as 3D texture coordinates
	// (array elements are float3's, which are 12
	// bytes in size and each represent a single
	// xyz triple)
	glClientActiveTexture(GL_TEXTURE5);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	#ifdef USE_PIECE_GEOMETRY_VBOS
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[VBO_STANGENTS]);
	glTexCoordPointer(3, GL_FLOAT, sizeof(float3), NULL);
	#else
	glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &(sTangents[0].x));
	#endif

	glClientActiveTexture(GL_TEXTURE6);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	#ifdef USE_PIECE_GEOMETRY_VBOS
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[VBO_TTANGENTS]);
	glTexCoordPointer(3, GL_FLOAT, sizeof(float3), NULL);
	#else
	glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &(tTangents[0].x));
	#endif

	glClientActiveTexture(GL_TEXTURE1);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	#ifdef USE_PIECE_GEOMETRY_VBOS
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[VBO_VTEXCOORS]);
	glTexCoordPointer(2, GL_FLOAT, sizeof(float2), NULL);
	#else
	glTexCoordPointer(2, GL_FLOAT, sizeof(float2), &(texcoors[0].x));
	#endif

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	#ifdef USE_PIECE_GEOMETRY_VBOS
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[VBO_VTEXCOORS]);
	glTexCoordPointer(2, GL_FLOAT, sizeof(float2), NULL);
	#else
	glTexCoordPointer(2, GL_FLOAT, sizeof(float2), &(texcoors[0].x));
	#endif

	glEnableClientState(GL_VERTEX_ARRAY);
	#ifdef USE_PIECE_GEOMETRY_VBOS
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[VBO_VERTICES]);
	glVertexPointer(3, GL_FLOAT, sizeof(float3), NULL);
	#else
	glVertexPointer(3, GL_FLOAT, sizeof(float3), &(vertices[0].x));
	#endif

	glEnableClientState(GL_NORMAL_ARRAY);
	#ifdef USE_PIECE_GEOMETRY_VBOS
	glBindBuffer(GL_ARRAY_BUFFER, vboIDs[VBO_VNORMALS]);
	glNormalPointer(GL_FLOAT, sizeof(float3), NULL);
	#else
	glNormalPointer(GL_FLOAT, sizeof(float3), &(vnormals[0].x));
	#endif

	#ifdef USE_PIECE_GEOMETRY_VBOS
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboIDs[VBO_VINDICES]);
	#endif

	switch (primitiveType) {
		case S3O_PRIMTYPE_TRIANGLES: {
			#ifdef USE_PIECE_GEOMETRY_VBOS
			glDrawElements(GL_TRIANGLES, vertexDrawIndices.size(), GL_UNSIGNED_INT, NULL);
			#else
			glDrawElements(GL_TRIANGLES, vertexDrawIndices.size(), GL_UNSIGNED_INT, &vertexDrawIndices[0]);
			#endif
		} break;
		case S3O_PRIMTYPE_TRIANGLE_STRIP: {
			#ifdef GLEW_NV_primitive_restart
			if (globalRendering->supportRestartPrimitive) {
				// this is not compiled into display lists, but executed immediately
				glPrimitiveRestartIndexNV(-1U);
				glEnableClientState(GL_PRIMITIVE_RESTART_NV);
			}
			#endif

			#ifdef USE_PIECE_GEOMETRY_VBOS
			glDrawElements(GL_TRIANGLE_STRIP, vertexDrawIndices.size(), GL_UNSIGNED_INT, NULL);
			#else
			glDrawElements(GL_TRIANGLE_STRIP, vertexDrawIndices.size(), GL_UNSIGNED_INT, &vertexDrawIndices[0]);
			#endif

			#ifdef GLEW_NV_primitive_restart
			if (globalRendering->supportRestartPrimitive) {
				glDisableClientState(GL_PRIMITIVE_RESTART_NV);
			}
			#endif
		} break;
		case S3O_PRIMTYPE_QUADS: {
			#ifdef USE_PIECE_GEOMETRY_VBOS
			glDrawElements(GL_QUADS, vertexDrawIndices.size(), GL_UNSIGNED_INT, NULL);
			#else
			glDrawElements(GL_QUADS, vertexDrawIndices.size(), GL_UNSIGNED_INT, &vertexDrawIndices[0]);
			#endif
		} break;
	}

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

	#ifdef USE_PIECE_GEOMETRY_VBOS
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	#endif
}

void SS3OPiece::SetMinMaxExtends()
{
	for (unsigned int n = 0; n < vertices.size(); n++) {
		mins.x = std::min(mins.x, (goffset.x + vertices[n].x));
		mins.y = std::min(mins.y, (goffset.y + vertices[n].y));
		mins.z = std::min(mins.z, (goffset.z + vertices[n].z));
		maxs.x = std::max(maxs.x, (goffset.x + vertices[n].x));
		maxs.y = std::max(maxs.y, (goffset.y + vertices[n].y));
		maxs.z = std::max(maxs.z, (goffset.z + vertices[n].z));
	}
}

void SS3OPiece::SetVertexTangents()
{
	if (isEmpty)
		return;

	// always allocate space if non-empty to simplify drawing code
	sTangents.resize(GetVertexCount(), ZeroVector);
	tTangents.resize(GetVertexCount(), ZeroVector);

	if (primitiveType == S3O_PRIMTYPE_QUADS)
		return;

	unsigned stride = 0;

	switch (primitiveType) {
		case S3O_PRIMTYPE_TRIANGLES: {
			stride = 3;
		} break;
		case S3O_PRIMTYPE_TRIANGLE_STRIP: {
			stride = 1;
		} break;
	}

	// for triangle strips, the piece vertex _indices_ are defined
	// by the draw order of the vertices numbered <v, v + 1, v + 2>
	// for v in [0, n - 2]
	const unsigned vrtMaxNr = (stride == 1)?
		vertexDrawIndices.size() - 2:
		vertexDrawIndices.size();

	// set the triangle-level S- and T-tangents
	for (unsigned vrtNr = 0; vrtNr < vrtMaxNr; vrtNr += stride) {
		bool flipWinding = false;

		if (primitiveType == S3O_PRIMTYPE_TRIANGLE_STRIP) {
			flipWinding = ((vrtNr & 1) == 1);
		}

		const int v0idx = vertexDrawIndices[vrtNr                      ];
		const int v1idx = vertexDrawIndices[vrtNr + (flipWinding? 2: 1)];
		const int v2idx = vertexDrawIndices[vrtNr + (flipWinding? 1: 2)];

		if (v1idx == -1 || v2idx == -1) {
			// not a valid triangle, skip
			// to start of next tri-strip
			vrtNr += 3; continue;
		}

		const float3& p0 = vertices[v0idx];
		const float3& p1 = vertices[v1idx];
		const float3& p2 = vertices[v2idx];

		const float2& tc0 = texcoors[v0idx];
		const float2& tc1 = texcoors[v1idx];
		const float2& tc2 = texcoors[v2idx];

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

		sTangents[v0idx] += sdir;
		sTangents[v1idx] += sdir;
		sTangents[v2idx] += sdir;

		tTangents[v0idx] += tdir;
		tTangents[v1idx] += tdir;
		tTangents[v2idx] += tdir;
	}

	// set the smoothed per-vertex tangents
	for (int vrtIdx = vertices.size() - 1; vrtIdx >= 0; vrtIdx--) {
		float3& n = vnormals[vrtIdx];
		float3& s = sTangents[vrtIdx];
		float3& t = tTangents[vrtIdx];
		int h = 1;

		if (math::isnan(n.x) || math::isnan(n.y) || math::isnan(n.z)) {
			n = float3(0.0f, 0.0f, 1.0f);
		}
		if (s == ZeroVector) { s = float3(1.0f, 0.0f, 0.0f); }
		if (t == ZeroVector) { t = float3(0.0f, 1.0f, 0.0f); }

		h = ((n.cross(s)).dot(t) < 0.0f)? -1: 1;
		s = (s - n * n.dot(s));
		s = s.SafeANormalize();
		t = (s.cross(n)) * h;

		// t = (s.cross(n));
		// h = ((s.cross(t)).dot(n) >= 0.0f)? 1: -1;
		// t = t * h;
	}
}



void SS3OPiece::Shatter(float pieceChance, int texType, int team, const float3& pos, const float3& speed) const
{
	// NOTE: is this still even possible for S3O pieces?
	if (texType <= 0)
		return;

	switch (primitiveType) {
		case S3O_PRIMTYPE_TRIANGLES: {
			for (size_t i = 0; i < vertexDrawIndices.size(); i += 3) {
				if (gu->RandFloat() > pieceChance)
					continue;

				SS3OVertex* verts = new SS3OVertex[4];
				const int offsets[4] = {0, 1, 1, 2};

				for (unsigned int j = 0; j < 4; j++) {
					verts[j].pos      = vertices[vertexDrawIndices[i + offsets[j]]];
					verts[j].normal   = vnormals[vertexDrawIndices[i + offsets[j]]];
					verts[j].textureX = texcoors[vertexDrawIndices[i + offsets[j]]].x;
					verts[j].textureY = texcoors[vertexDrawIndices[i + offsets[j]]].y;
				}

				ph->AddFlyingPiece(texType, team, pos, speed + gu->RandVector() * 2.0f, verts);
			}
		} break;
		case S3O_PRIMTYPE_TRIANGLE_STRIP: {
			// vertexDrawIndices can contain end-of-strip markers (-1U)
			for (size_t i = 2; i < vertexDrawIndices.size(); ) {
				if (gu->RandFloat() > pieceChance) { i += 1; continue; }
				if (vertexDrawIndices[i] == -1) { i += 3; continue; }

				SS3OVertex* verts = new SS3OVertex[4];
				const int offsets[4] = {-2, -1, -1, 0};

				for (unsigned int j = 0; j < 4; j++) {
					verts[j].pos      = vertices[vertexDrawIndices[i + offsets[j]]];
					verts[j].normal   = vnormals[vertexDrawIndices[i + offsets[j]]];
					verts[j].textureX = texcoors[vertexDrawIndices[i + offsets[j]]].x;
					verts[j].textureY = texcoors[vertexDrawIndices[i + offsets[j]]].y;
				}

				ph->AddFlyingPiece(texType, team, pos, speed + gu->RandVector() * 2.0f, verts);
				i += 1;
			}
		} break;
		case S3O_PRIMTYPE_QUADS: {
			for (size_t i = 0; i < vertexDrawIndices.size(); i += 4) {
				if (gu->RandFloat() > pieceChance)
					continue;

				SS3OVertex* verts = new SS3OVertex[4];

				for (unsigned int j = 0; j < 4; j++) {
					verts[j].pos      = vertices[vertexDrawIndices[i + j]];
					verts[j].normal   = vnormals[vertexDrawIndices[i + j]];
					verts[j].textureX = texcoors[vertexDrawIndices[i + j]].x;
					verts[j].textureY = texcoors[vertexDrawIndices[i + j]].y;
				}

				ph->AddFlyingPiece(texType, team, pos, speed + gu->RandVector() * 2.0f, verts);
			}
		} break;

		default: {
		} break;
	}
}
