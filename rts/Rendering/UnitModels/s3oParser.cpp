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
	unsigned char* fileBuf=new unsigned char[file.FileSize()];
	file.Read(fileBuf, file.FileSize());
	S3OHeader header;
	memcpy(&header,fileBuf,sizeof(header));
	header.swap();

	S3DModel *model = new S3DModel;
	model->type=MODELTYPE_S3O;
	model->numobjects=0;
	model->name=name;
	model->tex1=(char*)&fileBuf[header.texture1];
	model->tex2=(char*)&fileBuf[header.texture2];
	texturehandlerS3O->LoadS3OTexture(model);
	SS3OPiece* object = LoadPiece(fileBuf, header.rootPiece, model);
	object->type=MODELTYPE_S3O;

	FindMinMax(object);

	model->rootobject=object;
	model->radius = header.radius;
	model->height = header.height;
	model->relMidPos.x=header.midx;
	model->relMidPos.y=header.midy;
	model->relMidPos.z=header.midz;
	if(model->relMidPos.y<1)
		model->relMidPos.y=1;

	model->maxx=object->maxx;
	model->maxy=object->maxy;
	model->maxz=object->maxz;

	model->minx=object->minx;
	model->miny=object->miny;
	model->minz=object->minz;

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

void CS3OParser::FindMinMax(SS3OPiece* object)
{
	std::vector<S3DModelPiece*>::iterator si;

	for (si = object->childs.begin(); si != object->childs.end(); ++si) {
		FindMinMax(static_cast<SS3OPiece*>(*si));
	}

	float maxx = -1000.0f, maxy = -1000.0f, maxz = -1000.0f;
	float minx = 10000.0f, miny = 10000.0f, minz = 10000.0f;

	std::vector<SS3OVertex>::iterator vi;

	for (vi = object->vertices.begin(); vi != object->vertices.end(); ++vi) {
		maxx = std::max(maxx, vi->pos.x);
		maxy = std::max(maxy, vi->pos.y);
		maxz = std::max(maxz, vi->pos.z);

		minx = std::min(minx, vi->pos.x);
		miny = std::min(miny, vi->pos.y);
		minz = std::min(minz, vi->pos.z);
	}

	for (si = object->childs.begin(); si != object->childs.end(); ++si) {
		maxx = std::max(maxx, (*si)->offset.x + (*si)->maxx);
		maxy = std::max(maxy, (*si)->offset.y + (*si)->maxy);
		maxz = std::max(maxz, (*si)->offset.z + (*si)->maxz);

		minx = std::min(minx, (*si)->offset.x + (*si)->minx);
		miny = std::min(miny, (*si)->offset.y + (*si)->miny);
		minz = std::min(minz, (*si)->offset.z + (*si)->minz);
	}

	object->maxx = maxx;
	object->maxy = maxy;
	object->maxz = maxz;

	object->minx = minx;
	object->miny = miny;
	object->minz = minz;
}

void CS3OParser::Draw(S3DModelPiece *o)
{
	if (o->isEmpty) {
		return;
	}

	SS3OPiece* so = static_cast<SS3OPiece*>(o);
	SS3OVertex* s3ov = static_cast<SS3OVertex*>(&so->vertices[0]);

	// TODO (#1 -- Kloot)
	// glEnableVertexAttribArray(sTangentsIdx);
	// glEnableVertexAttribArray(tTangentsIdx);
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_NORMAL_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(SS3OVertex), &s3ov->pos.x);
	glNormalPointer(GL_FLOAT, sizeof(SS3OVertex), &s3ov->normal.x);
	glTexCoordPointer(2, GL_FLOAT, sizeof(SS3OVertex), &s3ov->textureX);

	// TODO (#2 -- Kloot)
	// glVertexAttribPointer(sTangentsIdx, 3, GL_FLOAT, GL_FALSE, 0, &so->sTangents[0]);
	// glVertexAttribPointer(tTangentsIdx, 3, GL_FLOAT, GL_FALSE, 0, &so->tTangents[0]);

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

	// TODO (#3 -- Kloot)
	// glDisableVertexAttribArray(sTangentsIdx);
	// glDisableVertexAttribArray(tTangentsIdx);

	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
}



void CS3OParser::SetVertexTangents(SS3OPiece* p)
{
	if (p->isEmpty || p->primitiveType == S3O_PRIMTYPE_QUADS) {
		return;
	}

	std::vector<SS3OTriangle> triangles;
	std::vector<std::vector<unsigned int> > verts2tris(p->vertexCount);

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
	unsigned vrtMaxNr = (stride == 1)?
		p->vertexDrawOrder.size() - 2:
		p->vertexDrawOrder.size();

	// set the triangle-level S- and T-tangents
	for (unsigned vrtNr = 0; vrtNr < vrtMaxNr; vrtNr += stride) {
		const int v0idx = p->vertexDrawOrder[vrtNr    ];
		const int v1idx = p->vertexDrawOrder[vrtNr + 1];
		const int v2idx = p->vertexDrawOrder[vrtNr + 2];

		const SS3OVertex* v0 = &p->vertices[v0idx];
		const SS3OVertex* v1 = &p->vertices[v1idx];
		const SS3OVertex* v2 = &p->vertices[v2idx];

		const float3 v1v0 = v1->pos - v0->pos;
		const float3 v2v0 = v2->pos - v0->pos;

		const float sd1 = v1->textureX - v0->textureX; // u1u0
		const float sd2 = v2->textureX - v0->textureX; // u2u0
		const float td1 = v1->textureY - v0->textureY; // v1v0
		const float td2 = v2->textureY - v0->textureY; // v2v0

		// if d is 0, texcoors are degenerate
		const float d = (sd1 * td2) - (sd2 * td1);
		const bool b = (d > -0.001f && d < 0.001f);
		const float r = b? 1.0f: 1.0f / d;

		const float3 sDir = (v1v0 * -td2 + v2v0 * td1) * r;
		const float3 tDir = (v1v0 * -sd2 + v2v0 * sd1) * r;

		SS3OTriangle tri;
			tri.v0idx = v0idx;
			tri.v1idx = v1idx;
			tri.v2idx = v2idx;
			tri.sTangent = sDir;
			tri.tTangent = tDir;
		triangles.push_back(tri);

		// save the triangle index
		verts2tris[v0idx].push_back(triangles.size() - 1);
		verts2tris[v1idx].push_back(triangles.size() - 1);
		verts2tris[v2idx].push_back(triangles.size() - 1);
	}

	// set the per-vertex tangents (for each vertex, this
	// is the average of the tangents of all the triangles
	// used by it)
	for (unsigned vrtNr = 0; vrtNr < p->vertexCount; vrtNr++) {
		for (unsigned triNr = 0; triNr < verts2tris[vrtNr].size(); triNr++) {
			const unsigned triIdx = verts2tris[vrtNr][triNr];
			const SS3OTriangle& tri = triangles[triIdx];

			p->sTangents[vrtNr] += tri.sTangent;
			p->tTangents[vrtNr] += tri.tTangent;
		}

		float3& s = p->sTangents[vrtNr]; // T
		float3& t = p->tTangents[vrtNr]; // B
		float3& n = p->vertices[vrtNr].normal;
		int h = 1; // handedness

		if (isnan(n.x) || isnan(n.y) || isnan(n.z)) {
			n = float3(0.0f, 1.0f, 0.0f);
		}

		// apply Gram-Schmidt since the smoothed
		// tangents likely do not form orthogonal
		// basis
		s = (s - (n * s.dot(n))).ANormalize();
		h = ((s.cross(t)).dot(n) >= 0.0f)? 1: -1;
		t = (n.cross(s)).ANormalize() * h;
	}
}
