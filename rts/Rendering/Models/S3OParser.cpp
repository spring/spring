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
		if (parent != NULL) {
			piece->parentName = parent->name;
		}

	piece->SetVertexCount(fp->numVertices);
	piece->SetVertexDrawIndexCount(fp->vertexTableSize);

	// retrieve each vertex
	int vertexOffset = fp->vertices;

	for (int a = 0; a < fp->numVertices; ++a) {
		Vertex* v = reinterpret_cast<Vertex*>(&buf[vertexOffset]);
			v->swap();

		SS3OVertex sv;
		sv.pos = float3(v->xpos, v->ypos, v->zpos);
		sv.normal = float3(v->xnormal, v->ynormal, v->znormal).SafeANormalize();
		sv.texCoord = float2(v->texu, v->texv);

		piece->SetVertex(a, sv);
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

	model->mins = std::min(piece->mins, model->mins);
	model->maxs = std::max(piece->maxs, model->maxs);

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

	//FIXME share 1 VBO for ALL models
	vboAttributes.Bind(GL_ARRAY_BUFFER);
	vboAttributes.Resize(vertices.size() * sizeof(SS3OVertex), GL_STATIC_DRAW, &vertices[0]);
	vboAttributes.Unbind();

	vboIndices.Bind(GL_ELEMENT_ARRAY_BUFFER);
	vboIndices.Resize(vertexDrawIndices.size() * sizeof(unsigned int), GL_STATIC_DRAW, &vertexDrawIndices[0]);
	vboIndices.Unbind();

	// NOTE: wasteful to keep these around, but still needed (eg. for Shatter())
	// vertices.clear();
	// vertexDrawIndices.clear();
}

void SS3OPiece::DrawForList() const
{
	if (isEmpty)
		return;
	
	vboAttributes.Bind(GL_ARRAY_BUFFER);
		glEnableClientState(GL_VERTEX_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(SS3OVertex), vboAttributes.GetPtr(offsetof(SS3OVertex, pos)));

		glEnableClientState(GL_NORMAL_ARRAY);
		glNormalPointer(GL_FLOAT, sizeof(SS3OVertex), vboAttributes.GetPtr(offsetof(SS3OVertex, normal)));

		glClientActiveTexture(GL_TEXTURE0);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(SS3OVertex), vboAttributes.GetPtr(offsetof(SS3OVertex, texCoord)));

		glClientActiveTexture(GL_TEXTURE1);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(2, GL_FLOAT, sizeof(SS3OVertex), vboAttributes.GetPtr(offsetof(SS3OVertex, texCoord)));

		glClientActiveTexture(GL_TEXTURE5);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(SS3OVertex), vboAttributes.GetPtr(offsetof(SS3OVertex, sTangent)));

		glClientActiveTexture(GL_TEXTURE6);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(SS3OVertex), vboAttributes.GetPtr(offsetof(SS3OVertex, tTangent)));
	vboAttributes.Unbind();

	vboIndices.Bind(GL_ELEMENT_ARRAY_BUFFER);
	switch (primitiveType) {
		case S3O_PRIMTYPE_TRIANGLES: {
			glDrawRangeElements(GL_TRIANGLES, 0, vertices.size() - 1, vertexDrawIndices.size(), GL_UNSIGNED_INT, vboIndices.GetPtr());
		} break;
		case S3O_PRIMTYPE_TRIANGLE_STRIP: {
			#ifdef GLEW_NV_primitive_restart
			if (globalRendering->supportRestartPrimitive) {
				// this is not compiled into display lists, but executed immediately
				glPrimitiveRestartIndexNV(-1U);
				glEnableClientState(GL_PRIMITIVE_RESTART_NV);
			}
			#endif

			glDrawRangeElements(GL_TRIANGLE_STRIP, 0, vertices.size() - 1, vertexDrawIndices.size(), GL_UNSIGNED_INT, vboIndices.GetPtr());

			#ifdef GLEW_NV_primitive_restart
			if (globalRendering->supportRestartPrimitive) {
				glDisableClientState(GL_PRIMITIVE_RESTART_NV);
			}
			#endif
		} break;
		case S3O_PRIMTYPE_QUADS: {
			glDrawRangeElements(GL_QUADS, 0, vertices.size() - 1, vertexDrawIndices.size(), GL_UNSIGNED_INT, vboIndices.GetPtr());
		} break;
	}
	vboIndices.Unbind();

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

void SS3OPiece::SetMinMaxExtends()
{
	for (std::vector<SS3OVertex>::const_iterator vi = vertices.begin(); vi != vertices.end(); ++vi) {
		mins = std::min(mins, (goffset + vi->pos));
		maxs = std::max(maxs, (goffset + vi->pos));
	}
}

void SS3OPiece::SetVertexTangents()
{
	if (isEmpty)
		return;

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

		const SS3OVertex* vrt0 = &vertices[v0idx];
		const SS3OVertex* vrt1 = &vertices[v1idx];
		const SS3OVertex* vrt2 = &vertices[v2idx];

		const float3& p0 = vrt0->pos;
		const float3& p1 = vrt1->pos;
		const float3& p2 = vrt2->pos;

		const float2& tc0 = vrt0->texCoord;
		const float2& tc1 = vrt1->texCoord;
		const float2& tc2 = vrt2->texCoord;

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

		vertices[v0idx].sTangent += sdir;
		vertices[v1idx].sTangent += sdir;
		vertices[v2idx].sTangent += sdir;

		vertices[v0idx].tTangent += tdir;
		vertices[v1idx].tTangent += tdir;
		vertices[v2idx].tTangent += tdir;
	}

	// set the smoothed per-vertex tangents
	for (int vrtIdx = vertices.size() - 1; vrtIdx >= 0; vrtIdx--) {
		float3& n = vertices[vrtIdx].normal;
		float3& s = vertices[vrtIdx].sTangent;
		float3& t = vertices[vrtIdx].tTangent;
		int h = 1;

		if (math::isnan(n.x) || math::isnan(n.y) || math::isnan(n.z)) {
			n = FwdVector;
		}
		if (s == ZeroVector) { s = RgtVector; }
		if (t == ZeroVector) { t =  UpVector; }

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
	//FIXME when all models share 1 vbo use that instead of recreating VAs

	// NOTE: is this still even possible for S3O pieces?
	if (texType <= 0)
		return;

	switch (primitiveType) {
		case S3O_PRIMTYPE_TRIANGLES: {
			for (size_t i = 0; i < vertexDrawIndices.size(); i += 3) {
				if (gu->RandFloat() > pieceChance)
					continue;

				SS3OVertex* verts = new SS3OVertex[4];

				verts[0] = vertices[vertexDrawIndices[i + 0]];
				verts[1] = vertices[vertexDrawIndices[i + 1]];
				verts[2] = vertices[vertexDrawIndices[i + 1]];
				verts[3] = vertices[vertexDrawIndices[i + 2]];

				projectileHandler->AddFlyingPiece(pos, speed + gu->RandVector() * 2.0f, team, texType, verts);
			}
		} break;
		case S3O_PRIMTYPE_TRIANGLE_STRIP: {
			// vertexDrawIndices can contain end-of-strip markers (-1U)
			for (size_t i = 2; i < vertexDrawIndices.size(); ) {
				if (gu->RandFloat() > pieceChance) { i += 1; continue; }
				if (vertexDrawIndices[i] == -1) { i += 3; continue; }

				SS3OVertex* verts = new SS3OVertex[4];

				verts[0] = vertices[vertexDrawIndices[i - 2]];
				verts[1] = vertices[vertexDrawIndices[i - 1]];
				verts[2] = vertices[vertexDrawIndices[i - 1]];
				verts[3] = vertices[vertexDrawIndices[i - 0]];

				projectileHandler->AddFlyingPiece(pos, speed + gu->RandVector() * 2.0f, team, texType, verts);
				i += 1;
			}
		} break;
		case S3O_PRIMTYPE_QUADS: {
			for (size_t i = 0; i < vertexDrawIndices.size(); i += 4) {
				if (gu->RandFloat() > pieceChance)
					continue;

				SS3OVertex* verts = new SS3OVertex[4];

				verts[0] = vertices[vertexDrawIndices[i + 0]];
				verts[1] = vertices[vertexDrawIndices[i + 1]];
				verts[2] = vertices[vertexDrawIndices[i + 2]];
				verts[3] = vertices[vertexDrawIndices[i + 3]];

				projectileHandler->AddFlyingPiece(pos, speed + gu->RandVector() * 2.0f, team, texType, verts);
			}
		} break;

		default: {
		} break;
	}
}
