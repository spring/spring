/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <locale>
#include <stdexcept>
#include "mmgr.h"

#include "S3OParser.h"
#include "s3o.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/COB/CobInstance.h"
#include "System/Exceptions.h"
#include "System/GlobalUnsynced.h"
#include "System/Util.h"
#include "System/Vec2.h"
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
		model->numobjects = 0;
		model->tex1 = (char*) &fileBuf[header.texture1];
		model->tex2 = (char*) &fileBuf[header.texture2];
		model->mins = DEF_MIN_SIZE;
		model->maxs = DEF_MAX_SIZE;
	texturehandlerS3O->LoadS3OTexture(model);

	SS3OPiece* rootPiece = LoadPiece(model, NULL, fileBuf, header.rootPiece);

	model->rootobject = rootPiece;
	model->radius = header.radius;
	model->height = header.height;

	model->relMidPos = float3(header.midx, header.midy, header.midz);
	model->relMidPos.y = std::max(model->relMidPos.y, 1.0f); // ?

	delete[] fileBuf;
	return model;
}

SS3OPiece* CS3OParser::LoadPiece(S3DModel* model, SS3OPiece* parent, unsigned char* buf, int offset)
{
	model->numobjects++;

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
		piece->vertices.reserve(fp->numVertices);
		piece->vertexDrawOrder.reserve((size_t)(fp->vertexTableSize * 1.1f)); //1.1f is just a guess (check below)

	// retrieve each vertex
	int vertexOffset = fp->vertices;

	for (int a = 0; a < fp->numVertices; ++a) {
		((Vertex*) &buf[vertexOffset])->swap();

		SS3OVertex* v = (SS3OVertex*) &buf[vertexOffset];
			v->normal.ANormalize();
		piece->vertices.push_back(*v);

		vertexOffset += sizeof(Vertex);
	}


	// retrieve the draw order for the vertices
	int vertexTableOffset = fp->vertexTable;

	for (int a = 0; a < fp->vertexTableSize; ++a) {
		int vertexDrawIdx = swabdword(*(int*) &buf[vertexTableOffset]);

		piece->vertexDrawOrder.push_back(vertexDrawIdx);
		vertexTableOffset += sizeof(int);

		// -1 == 0xFFFFFFFF (U)
		if (vertexDrawIdx == -1 && a != fp->vertexTableSize - 1) {
			// for triangle strips
			piece->vertexDrawOrder.push_back(vertexDrawIdx);

			vertexDrawIdx = swabdword(*(int*) &buf[vertexTableOffset]);
			piece->vertexDrawOrder.push_back(vertexDrawIdx);
		}
	}

	piece->isEmpty = piece->vertexDrawOrder.empty();
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

	piece->colvol = new CollisionVolume("box", cvScales, cvOffset * 0.5f, CollisionVolume::COLVOL_HITTEST_CONT);


	int childTableOffset = fp->childs;

	for (int a = 0; a < fp->numChilds; ++a) {
		int childOffset = swabdword(*(int*) &buf[childTableOffset]);

		SS3OPiece* childPiece = LoadPiece(model, piece, buf, childOffset);
		piece->childs.push_back(childPiece);

		childTableOffset += sizeof(int);
	}

	return piece;
}






void SS3OPiece::DrawList() const
{
	if (isEmpty) {
		return;
	}

	const SS3OVertex* s3ov = &vertices[0];

	// pass the tangents as 3D texture coordinates
	// (array elements are float3's, which are 12
	// bytes in size and each represent a single
	// xyz triple)
	if (!sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE5);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &sTangents[0].x);

		glClientActiveTexture(GL_TEXTURE6);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &tTangents[0].x);
	}

	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(SS3OVertex), &s3ov->textureX);

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(SS3OVertex), &s3ov->textureX);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(SS3OVertex), &s3ov->pos.x);

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(SS3OVertex), &s3ov->normal.x);

	switch (primitiveType) {
		case S3O_PRIMTYPE_TRIANGLES:
			glDrawElements(GL_TRIANGLES, vertexDrawOrder.size(), GL_UNSIGNED_INT, &vertexDrawOrder[0]);
			break;
		case S3O_PRIMTYPE_TRIANGLE_STRIP:
			glDrawElements(GL_TRIANGLE_STRIP, vertexDrawOrder.size(), GL_UNSIGNED_INT, &vertexDrawOrder[0]);
			break;
		case S3O_PRIMTYPE_QUADS:
			glDrawElements(GL_QUADS, vertexDrawOrder.size(), GL_UNSIGNED_INT, &vertexDrawOrder[0]);
			break;
	}

	if (!sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE6);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glClientActiveTexture(GL_TEXTURE5);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	glClientActiveTextureARB(GL_TEXTURE1_ARB);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}

void SS3OPiece::SetMinMaxExtends()
{
	for (std::vector<SS3OVertex>::const_iterator vi = vertices.begin(); vi != vertices.end(); ++vi) {
		mins.x = std::min(mins.x, (goffset.x + vi->pos.x));
		mins.y = std::min(mins.y, (goffset.y + vi->pos.y));
		mins.z = std::min(mins.z, (goffset.z + vi->pos.z));
		maxs.x = std::max(maxs.x, (goffset.x + vi->pos.x));
		maxs.y = std::max(maxs.y, (goffset.y + vi->pos.y));
		maxs.z = std::max(maxs.z, (goffset.z + vi->pos.z));
	}
}

void SS3OPiece::SetVertexTangents()
{
	if (isEmpty || primitiveType == S3O_PRIMTYPE_QUADS) {
		return;
	}

	sTangents.resize(GetVertexCount(), ZeroVector);
	tTangents.resize(GetVertexCount(), ZeroVector);

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
		vertexDrawOrder.size() - 2:
		vertexDrawOrder.size();

	// set the triangle-level S- and T-tangents
	for (unsigned vrtNr = 0; vrtNr < vrtMaxNr; vrtNr += stride) {
		bool flipWinding = false;

		if (primitiveType == S3O_PRIMTYPE_TRIANGLE_STRIP) {
			flipWinding = ((vrtNr & 1) == 1);
		}

		const int v0idx = vertexDrawOrder[vrtNr                      ];
		const int v1idx = vertexDrawOrder[vrtNr + (flipWinding? 2: 1)];
		const int v2idx = vertexDrawOrder[vrtNr + (flipWinding? 1: 2)];

		if (v1idx == -1 || v2idx == -1) {
			// not a valid triangle, skip
			// to start of next tri-strip
			vrtNr += 3; continue;
		}

		const SS3OVertex* vrt0 = &vertices[v0idx];
		const SS3OVertex* vrt1 = &vertices[v1idx];
		const SS3OVertex* vrt2 = &vertices[v2idx];

		const float3& p0 = vrt0->pos;
		const float3& p1 = vrt1->pos;
		const float3& p2 = vrt2->pos;

		const float2 tc0(vrt0->textureX, vrt0->textureY);
		const float2 tc1(vrt1->textureX, vrt1->textureY);
		const float2 tc2(vrt2->textureX, vrt2->textureY);

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
		float3& n = vertices[vrtIdx].normal;
		float3& s = sTangents[vrtIdx];
		float3& t = tTangents[vrtIdx];
		int h = 1;

		if (isnan(n.x) || isnan(n.y) || isnan(n.z)) {
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
	switch (primitiveType) {
		case 0: {
			// GL_TRIANGLES
			for (size_t i = 0; i < vertexDrawOrder.size(); i += 3) {
				if (gu->usRandFloat() > pieceChance)
					continue;

				SS3OVertex* verts = new SS3OVertex[4];

				verts[0] = vertices[vertexDrawOrder[i + 0]];
				verts[1] = vertices[vertexDrawOrder[i + 1]];
				verts[2] = vertices[vertexDrawOrder[i + 1]];
				verts[3] = vertices[vertexDrawOrder[i + 2]];

				ph->AddFlyingPiece(texType, team, pos, speed + gu->usRandVector() * 2.0f, verts);
			}
		} break;
		case 1: {
			// GL_TRIANGLE_STRIP
			for (size_t i = 2; i < vertexDrawOrder.size(); i++){
				if (gu->usRandFloat() > pieceChance)
					continue;

				SS3OVertex* verts = new SS3OVertex[4];

				verts[0] = vertices[vertexDrawOrder[i - 2]];
				verts[1] = vertices[vertexDrawOrder[i - 1]];
				verts[2] = vertices[vertexDrawOrder[i - 1]];
				verts[3] = vertices[vertexDrawOrder[i - 0]];

				ph->AddFlyingPiece(texType, team, pos, speed + gu->usRandVector() * 2.0f, verts);
			}
		} break;
		case 2: {
			// GL_QUADS
			for (size_t i = 0; i < vertexDrawOrder.size(); i += 4) {
				if (gu->usRandFloat() > pieceChance)
					continue;

				SS3OVertex* verts = new SS3OVertex[4];

				verts[0] = vertices[vertexDrawOrder[i + 0]];
				verts[1] = vertices[vertexDrawOrder[i + 1]];
				verts[2] = vertices[vertexDrawOrder[i + 2]];
				verts[3] = vertices[vertexDrawOrder[i + 3]];

				ph->AddFlyingPiece(texType, team, pos, speed + gu->usRandVector() * 2.0f, verts);
			}
		} break;

		default: {
		} break;
	}
}
