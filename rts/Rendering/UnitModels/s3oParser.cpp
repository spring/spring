// s3oParser.cpp: implementation of the Cs3oParser class.
//
//////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <locale>
#include <stdexcept>
#include "mmgr.h"

#include "s3oParser.h"
#include "Rendering/GL/myGL.h"
#include "FileSystem/FileHandler.h"
#include "s3o.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Rendering/Textures/S3OTextureHandler.h"
#include "Platform/byteorder.h"
#include "Platform/errorhandler.h"
#include "Util.h"
#include "Exceptions.h"
#include "LogOutput.h"


S3DModel* CS3OParser::Load(std::string name)
{
	CFileHandler file(name);
	if (!file.FileExists()) {
		throw content_error("File not found: "+name);
	}

	unsigned char* fileBuf = new unsigned char[file.FileSize()];
	file.Read(fileBuf, file.FileSize());
	S3OHeader header;
	memcpy(&header, fileBuf, sizeof(header));
	header.swap();

	S3DModel* model = new S3DModel;
	model->type = MODELTYPE_S3O;
	model->numobjects = 0;
	model->name = name;
	model->tex1 = (char*) &fileBuf[header.texture1];
	model->tex2 = (char*) &fileBuf[header.texture2];
	texturehandlerS3O->LoadS3OTexture(model);

	SS3OPiece* rootPiece = LoadPiece(fileBuf, header.rootPiece, model);
	rootPiece->type = MODELTYPE_S3O;

	FindMinMax(rootPiece);

	model->rootobject = rootPiece;
	model->radius = header.radius;
	model->height = header.height;

	model->relMidPos.x = header.midx;
	model->relMidPos.y = header.midy;
	model->relMidPos.z = header.midz;

	model->relMidPos.y = std::max(model->relMidPos.y, 1.0f); // ?

	model->maxx = rootPiece->maxx;
	model->maxy = rootPiece->maxy;
	model->maxz = rootPiece->maxz;

	model->minx = rootPiece->minx;
	model->miny = rootPiece->miny;
	model->minz = rootPiece->minz;

	delete[] fileBuf;
	return model;
}

SS3OPiece* CS3OParser::LoadPiece(unsigned char* buf, int offset, S3DModel* model)
{
	model->numobjects++;

	SS3OPiece* piece = new SS3OPiece;
	piece->type = MODELTYPE_S3O;

	Piece* fp = (Piece*)&buf[offset];
	fp->swap(); // Does it matter we mess with the original buffer here? Don't hope so.

	piece->offset.x = fp->xoffset;
	piece->offset.y = fp->yoffset;
	piece->offset.z = fp->zoffset;
	piece->primitiveType = fp->primitiveType;
	piece->name = (char*) &buf[fp->name];


	// retrieve each vertex
	int vertexOffset = fp->vertices;

	for (int a = 0; a < fp->numVertices; ++a) {
		((Vertex*) &buf[vertexOffset])->swap();

		SS3OVertex* v = (SS3OVertex*) &buf[vertexOffset];
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
	piece->vertexCount = piece->vertices.size();

	SetVertexTangents(piece);

	int childTableOffset = fp->childs;

	for (int a = 0; a < fp->numChilds; ++a) {
		int childOffset = swabdword(*(int*) &buf[childTableOffset]);

		SS3OPiece* childPiece = LoadPiece(buf, childOffset, model);
		piece->childs.push_back(childPiece);

		childTableOffset += sizeof(int);
	}

	return piece;
}

void CS3OParser::FindMinMax(SS3OPiece* o) const
{
	std::vector<S3DModelPiece*>::iterator si;

	for (si = o->childs.begin(); si != o->childs.end(); ++si) {
		FindMinMax(static_cast<SS3OPiece*>(*si));
	}

	float maxx = -1000.0f, maxy = -1000.0f, maxz = -1000.0f;
	float minx = 10000.0f, miny = 10000.0f, minz = 10000.0f;

	std::vector<SS3OVertex>::iterator vi;

	for (vi = o->vertices.begin(); vi != o->vertices.end(); ++vi) {
		maxx = std::max(maxx, vi->pos.x);
		maxy = std::max(maxy, vi->pos.y);
		maxz = std::max(maxz, vi->pos.z);

		minx = std::min(minx, vi->pos.x);
		miny = std::min(miny, vi->pos.y);
		minz = std::min(minz, vi->pos.z);
	}

	for (si = o->childs.begin(); si != o->childs.end(); ++si) {
		maxx = std::max(maxx, (*si)->offset.x + (*si)->maxx);
		maxy = std::max(maxy, (*si)->offset.y + (*si)->maxy);
		maxz = std::max(maxz, (*si)->offset.z + (*si)->maxz);

		minx = std::min(minx, (*si)->offset.x + (*si)->minx);
		miny = std::min(miny, (*si)->offset.y + (*si)->miny);
		minz = std::min(minz, (*si)->offset.z + (*si)->minz);
	}

	o->maxx = maxx;
	o->maxy = maxy;
	o->maxz = maxz;

	o->minx = minx;
	o->miny = miny;
	o->minz = minz;

	const float3 cvScales((o->maxx - o->minx),        (o->maxy - o->miny),        (o->maxz - o->minz)       );
	const float3 cvOffset((o->maxx + o->minx) * 0.5f, (o->maxy + o->miny) * 0.5f, (o->maxz + o->minz) * 0.5f);

	o->colvol = new CollisionVolume("box", cvScales, cvOffset, COLVOL_TEST_CONT);
	o->colvol->Enable();
}

void CS3OParser::Draw(const S3DModelPiece* o) const
{
	if (o->isEmpty) {
		return;
	}

	const SS3OPiece* so = static_cast<const SS3OPiece*>(o);
	const SS3OVertex* s3ov = static_cast<const SS3OVertex*>(&so->vertices[0]);


	// pass the tangents as 3D texture coordinates
	// (array elements are float3's, which are 12
	// bytes in size and each represent a single
	// xyz triple)
	// TODO: test if we have this many texunits
	// (if not, could only send the s-tangents)?
	if (!so->sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE5);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &so->sTangents[0].x);

		glClientActiveTexture(GL_TEXTURE6);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glTexCoordPointer(3, GL_FLOAT, sizeof(float3), &so->tTangents[0].x);
	}

	glClientActiveTexture(GL_TEXTURE0);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glTexCoordPointer(2, GL_FLOAT, sizeof(SS3OVertex), &s3ov->textureX);

	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(SS3OVertex), &s3ov->pos.x);

	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, sizeof(SS3OVertex), &s3ov->normal.x);

	switch (so->primitiveType) {
		case S3O_PRIMTYPE_TRIANGLES:
			glDrawElements(GL_TRIANGLES, so->vertexDrawOrder.size(), GL_UNSIGNED_INT, &so->vertexDrawOrder[0]);
			break;
		case S3O_PRIMTYPE_TRIANGLE_STRIP:
			glDrawElements(GL_TRIANGLE_STRIP, so->vertexDrawOrder.size(), GL_UNSIGNED_INT, &so->vertexDrawOrder[0]);
			break;
		case S3O_PRIMTYPE_QUADS:
			glDrawElements(GL_QUADS, so->vertexDrawOrder.size(), GL_UNSIGNED_INT, &so->vertexDrawOrder[0]);
			break;
	}

	if (!so->sTangents.empty()) {
		glClientActiveTexture(GL_TEXTURE6);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);

		glClientActiveTexture(GL_TEXTURE5);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	}

	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
}



void CS3OParser::SetVertexTangents(SS3OPiece* p)
{
	if (p->isEmpty || p->primitiveType == S3O_PRIMTYPE_QUADS) {
		return;
	}

	p->sTangents.resize(p->vertexCount, ZeroVector);
	p->tTangents.resize(p->vertexCount, ZeroVector);

	unsigned stride = 0;

	switch (p->primitiveType) {
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
		p->vertexDrawOrder.size() - 2:
		p->vertexDrawOrder.size();

	// set the triangle-level S- and T-tangents
	for (unsigned vrtNr = 0; vrtNr < vrtMaxNr; vrtNr += stride) {
		bool flipWinding = false;

		if (p->primitiveType == S3O_PRIMTYPE_TRIANGLE_STRIP) {
			flipWinding = ((vrtNr & 1) == 1); 
		}

		const int v0idx = p->vertexDrawOrder[vrtNr                      ];
		const int v1idx = p->vertexDrawOrder[vrtNr + (flipWinding? 2: 1)];
		const int v2idx = p->vertexDrawOrder[vrtNr + (flipWinding? 1: 2)];

		if (v1idx == -1 || v2idx == -1) {
			// not a valid triangle, skip
			// to start of next tri-strip
			vrtNr += 3; continue;
		}

		const SS3OVertex* vrt0 = &p->vertices[v0idx];
		const SS3OVertex* vrt1 = &p->vertices[v1idx];
		const SS3OVertex* vrt2 = &p->vertices[v2idx];

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

		p->sTangents[v0idx] += sdir;
		p->sTangents[v1idx] += sdir;
		p->sTangents[v2idx] += sdir;

		p->tTangents[v0idx] += tdir;
		p->tTangents[v1idx] += tdir;
		p->tTangents[v2idx] += tdir;
	}

	// set the smoothed per-vertex tangents
	for (unsigned vrtIdx = 0; vrtIdx < p->vertices.size(); vrtIdx++) {
		float3& n = p->vertices[vrtIdx].normal;
		float3& s = p->sTangents[vrtIdx];
		float3& t = p->tTangents[vrtIdx];
		int h = 1;

		if (isnan(n.x) || isnan(n.y) || isnan(n.z)) {
			n = float3(0.0f, 1.0f, 0.0f);
		}

		h = ((n.cross(s)).dot(t) < 0.0f)? -1: 1;
		s = (s - n * n.dot(s)).ANormalize();
		t = (s.cross(n)) * h;

		// t = (s.cross(n));
		// h = ((s.cross(t)).dot(n) >= 0.0f)? 1: -1;
		// t = t * h;
	}
}
