/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include <vector>
#include <set>
#include <algorithm>
#include <cctype>
#include <locale>
#include <stdexcept>
#include "mmgr.h"

#include "3DOParser.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Units/COB/CobInstance.h"
#include "System/Util.h"
#include "System/Exceptions.h"
#include "System/GlobalUnsynced.h"
#include "System/LogOutput.h"
#include "System/Matrix44f.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Platform/byteorder.h"
#include <boost/cstdint.hpp>

using namespace std;

static const float  scaleFactor = 1 / (65536.0f);
static const float3 DownVector  = -UpVector;

static const float3 DEF_MIN_SIZE( 10000.0f,  10000.0f,  10000.0f);
static const float3 DEF_MAX_SIZE(-10000.0f, -10000.0f, -10000.0f);

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define READ_3DOBJECT(o)					\
do {								\
	unsigned int __tmp;					\
	unsigned short __isize = sizeof(unsigned int);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).VersionSignature = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).NumberOfVertices = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).NumberOfPrimitives = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).SelectionPrimitive = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).XFromParent = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).YFromParent = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).ZFromParent = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).OffsetToObjectName = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(o).Always_0 = (int)swabdword(__tmp);			\
	SimStreamRead(&__tmp,__isize);				\
	(o).OffsetToVertexArray = (int)swabdword(__tmp);	\
	SimStreamRead(&__tmp,__isize);				\
	(o).OffsetToPrimitiveArray = (int)swabdword(__tmp);	\
	SimStreamRead(&__tmp,__isize);				\
	(o).OffsetToSiblingObject = (int)swabdword(__tmp);	\
	SimStreamRead(&__tmp,__isize);				\
	(o).OffsetToChildObject = (int)swabdword(__tmp);	\
} while (0)


#define READ_VERTEX(v)					\
do {							\
	unsigned int __tmp;				\
	unsigned short __isize = sizeof(unsigned int);	\
	SimStreamRead(&__tmp,__isize);			\
	(v).x = (int)swabdword(__tmp);			\
	SimStreamRead(&__tmp,__isize);			\
	(v).y = (int)swabdword(__tmp);			\
	SimStreamRead(&__tmp,__isize);			\
	(v).z = (int)swabdword(__tmp);			\
} while (0)


#define READ_PRIMITIVE(p)					\
do {								\
	unsigned int __tmp;					\
	unsigned short __isize = sizeof(unsigned int);		\
	SimStreamRead(&__tmp,__isize);				\
	(p).PaletteEntry = (int)swabdword(__tmp);		\
	SimStreamRead(&__tmp,__isize);				\
	(p).NumberOfVertexIndexes = (int)swabdword(__tmp);	\
	SimStreamRead(&__tmp,__isize);				\
	(p).Always_0 = (int)swabdword(__tmp);			\
	SimStreamRead(&__tmp,__isize);				\
	(p).OffsetToVertexIndexArray = (int)swabdword(__tmp);	\
	SimStreamRead(&__tmp,__isize);				\
	(p).OffsetToTextureName = (int)swabdword(__tmp);	\
	SimStreamRead(&__tmp,__isize);				\
	(p).Unknown_1 = (int)swabdword(__tmp);			\
	SimStreamRead(&__tmp,__isize);				\
	(p).Unknown_2 = (int)swabdword(__tmp);			\
	SimStreamRead(&__tmp,__isize);				\
	(p).Unknown_3 = (int)swabdword(__tmp);			\
} while (0)



C3DOParser::C3DOParser()
{
	CFileHandler file("unittextures/tatex/teamtex.txt");
	CSimpleParser parser(file);

	while (!file.Eof()) {
		teamtex.insert(StringToLower(parser.GetLine()));
	}

	fileBuf = NULL;
}


S3DModel* C3DOParser::Load(const string& name)
{
	CFileHandler file(name);
	if (!file.FileExists()) {
		throw content_error("[3DOParser] could not find model-file " + name);
	}

	fileBuf = new unsigned char[file.FileSize()];
	assert(fileBuf);

	const int readn = file.Read(fileBuf, file.FileSize());

	if (readn == 0) {
		delete[] fileBuf;
		fileBuf = NULL;
		throw content_error("[3DOParser] Failed to read file " + name);
	}

	S3DModel* model = new S3DModel;
		model->name = name;
		model->type = MODELTYPE_3DO;
		model->textureType = 0;
		model->numobjects  = 0;
		model->mins = DEF_MIN_SIZE;
		model->maxs = DEF_MAX_SIZE;
		model->radius = 0.0f;
		model->height = 0.0f;

	S3DOPiece* rootPiece = LoadPiece(model, 0, NULL, &model->numobjects);

	model->SetRootPiece(rootPiece);
	model->radius =
		(((model->maxs.x - model->mins.x) * 0.5f) * ((model->maxs.x - model->mins.x) * 0.5f)) +
		(((model->maxs.y - model->mins.y) * 0.5f) * ((model->maxs.y - model->mins.y) * 0.5f)) +
		(((model->maxs.z - model->mins.z) * 0.5f) * ((model->maxs.z - model->mins.z) * 0.5f));
	model->radius = math::sqrt(model->radius);
	model->height = model->maxs.y - model->mins.y;
	// model->height = model->radius * 2.0f;
	model->relMidPos = (model->maxs - model->mins) * 0.5f;
	model->relMidPos.x = 0.0f; // ?
	model->relMidPos.z = 0.0f; // ?

	delete[] fileBuf;
	fileBuf = NULL;
	return model;
}


void C3DOParser::GetVertexes(_3DObject* o, S3DOPiece* object)
{
	curOffset = o->OffsetToVertexArray;
	object->vertices.reserve(o->NumberOfVertices);

	for (int a = 0; a < o->NumberOfVertices; a++) {
		float3 v;
		READ_VERTEX(v);

		S3DOVertex vertex;
		v *= scaleFactor;
		v.z = -v.z;
		vertex.pos = v;
		object->vertices.push_back(vertex);
	}
}


void C3DOParser::GetPrimitives(S3DOPiece* obj, int pos, int num, int excludePrim)
{
	map<int,int> prevHashes;

	for(int a=0;a<num;a++){
		if(excludePrim==a){
			continue;
		}
		curOffset=pos+a*sizeof(_Primitive);
		_Primitive p;

		READ_PRIMITIVE(p);
		S3DOPrimitive sp;
		sp.numVertex=p.NumberOfVertexIndexes;

		if(sp.numVertex<3)
			continue;

		sp.vertices.reserve(sp.numVertex);
		sp.normals.reserve(sp.numVertex);

		curOffset=p.OffsetToVertexIndexArray;
		boost::uint16_t w;

		list<int> orderVert;
		for(int b=0;b<sp.numVertex;b++){
			SimStreamRead(&w,2);
			w = swabword(w);
			sp.vertices.push_back(w);
			orderVert.push_back(w);
		}
		orderVert.sort();
		int vertHash=0;

		for(list<int>::iterator vi=orderVert.begin();vi!=orderVert.end();++vi)
			vertHash=(vertHash+(*vi))*(*vi);

		sp.texture=0;
		if(p.OffsetToTextureName!=0)
		{
			string texture = GetText(p.OffsetToTextureName);
			StringToLowerInPlace(texture);

			if(teamtex.find(texture) != teamtex.end())
				sp.texture=texturehandler3DO->Get3DOTexture(texture);
			else
				sp.texture=texturehandler3DO->Get3DOTexture(texture + "00");

			if(sp.texture==0)
				LogObject() << "Parser couldnt get texture " << GetText(p.OffsetToTextureName).c_str() << "\n";
		} else {
			char t[50];
			sprintf(t,"ta_color%i",p.PaletteEntry);
			sp.texture=texturehandler3DO->Get3DOTexture(t);
		}

		float3 n = -(obj->vertices[sp.vertices[1]].pos - obj->vertices[sp.vertices[0]].pos).cross(obj->vertices[sp.vertices[2]].pos - obj->vertices[sp.vertices[0]].pos);
		n.SafeNormalize();
		sp.normal = n;
		sp.normals.insert(sp.normals.begin(), sp.numVertex, n);

		// sometimes there are more than one selection primitive (??)
		if (n.dot(DownVector) > 0.99f) {
			bool ignore=true;

			if(sp.numVertex!=4) {
				ignore=false;
			} else {
				float3 s1=obj->vertices[sp.vertices[0]].pos-obj->vertices[sp.vertices[1]].pos;
				float3 s2=obj->vertices[sp.vertices[1]].pos-obj->vertices[sp.vertices[2]].pos;
				if(s1.SqLength()<900 || s2.SqLength()<900)
					ignore=false;

				if (ignore) {
					for(int a=0;a<sp.numVertex;++a) {
						if(obj->vertices[sp.vertices[a]].pos.y>0) {
							ignore=false;
							break;
						}
					}
				}
			}

			if(ignore)
				continue;
		}

		map<int,int>::iterator phi;
		if((phi=prevHashes.find(vertHash))!=prevHashes.end()){
			if(n.y>0)
				obj->prims[phi->second]=sp;
			continue;
		} else {
			prevHashes[vertHash]=obj->prims.size();
			obj->prims.push_back(sp);
			obj->isEmpty = false;
		}
		curOffset = p.OffsetToVertexIndexArray;

		for (int b = 0; b < sp.numVertex; b++) {
			SimStreamRead(&w, 2);
			w = swabword(w);
			obj->vertices[w].prims.push_back(obj->prims.size() - 1);
		}
	}
}


void C3DOParser::CalcNormals(S3DOPiece* o) const
{
	for (std::vector<S3DOPrimitive>::iterator ps = o->prims.begin(); ps != o->prims.end(); ps++) {
		for (int a = 0; a < ps->numVertex; ++a) {
			S3DOVertex* vertex = &o->vertices[ps->vertices[a]];
			float3 vnormal(0.0f, 0.0f, 0.0f);

			for (std::vector<int>::iterator pi = vertex->prims.begin(); pi != vertex->prims.end(); ++pi) {
				if (ps->normal.dot(o->prims[*pi].normal) > 0.45f) {
					vnormal += o->prims[*pi].normal;
				}
			}

			vnormal.SafeNormalize();
			ps->normals[a] = vnormal;
		}
	}
}


std::string C3DOParser::GetText(int pos)
{
	curOffset = pos;
	char c;
	std::string s;

	SimStreamRead(&c, 1);

	while (c != 0) {
		s += c;
		SimStreamRead(&c, 1);
	}

	return s;
}


S3DOPiece* C3DOParser::LoadPiece(S3DModel* model, int pos, S3DOPiece* parent, int* numobj)
{
	(*numobj)++;

	_3DObject me;

	curOffset = pos;
	READ_3DOBJECT(me);

	std::string s = GetText(me.OffsetToObjectName);
	StringToLowerInPlace(s);

	S3DOPiece* piece = new S3DOPiece();
		piece->name = s;
		piece->parent = parent;
		piece->dispListID = 0;
		piece->type = MODELTYPE_3DO;

		piece->mins = DEF_MIN_SIZE;
		piece->maxs = DEF_MAX_SIZE;
		piece->offset.x =  me.XFromParent * scaleFactor;
		piece->offset.y =  me.YFromParent * scaleFactor;
		piece->offset.z = -me.ZFromParent * scaleFactor;
		piece->goffset = piece->offset + ((parent != NULL)? parent->goffset: ZeroVector);

	GetVertexes(&me, piece);
	GetPrimitives(piece, me.OffsetToPrimitiveArray, me.NumberOfPrimitives, ((pos == 0)? me.SelectionPrimitive: -1));
	CalcNormals(piece);
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
	const float radiusSq =
		((cvScales.x * 0.5f) * (cvScales.x * 0.5f)) +
		((cvScales.y * 0.5f) * (cvScales.y * 0.5f)) +
		((cvScales.z * 0.5f) * (cvScales.z * 0.5f));

	piece->radius = math::sqrt(radiusSq);
	piece->relMidPos = cvOffset * 0.5f;
	piece->colvol = new CollisionVolume("box", cvScales, cvOffset * 0.5f, CollisionVolume::COLVOL_HITTEST_CONT);


	if (me.OffsetToChildObject > 0) {
		piece->childs.push_back(LoadPiece(model, me.OffsetToChildObject, piece, numobj));
	}

	piece->isEmpty = (piece->prims.size() < 1);

	if (me.OffsetToSiblingObject > 0) {
		parent->childs.push_back(LoadPiece(model, me.OffsetToSiblingObject, parent, numobj));
	}

	return piece;
}



void C3DOParser::SimStreamRead(void* buf, int length)
{
	memcpy(buf, &fileBuf[curOffset], length);
	curOffset += length;
}






void S3DOPiece::DrawList() const
{
	if (isEmpty) {
		return;
	}

	// note: do not use more than two VA's
	// via GetVertexArray(), it wraps around
	CVertexArray* va1 = GetVertexArray();
	CVertexArray* va2 = GetVertexArray();
	va1->Initialize();
	va2->Initialize();

	// glFrontFace(GL_CW);
	for (std::vector<S3DOPrimitive>::const_iterator ps = prims.begin(); ps != prims.end(); ++ps) {
		C3DOTextureHandler::UnitTexture* tex = ps->texture;

		if (ps->numVertex == 4) {
			va1->AddVertexTN(vertices[ps->vertices[0]].pos, tex->xstart, tex->ystart, ps->normals[0]);
			va1->AddVertexTN(vertices[ps->vertices[1]].pos, tex->xend,   tex->ystart, ps->normals[1]);
			va1->AddVertexTN(vertices[ps->vertices[2]].pos, tex->xend,   tex->yend,   ps->normals[2]);
			va1->AddVertexTN(vertices[ps->vertices[3]].pos, tex->xstart, tex->yend,   ps->normals[3]);
		} else if (ps->numVertex == 3) {
			va2->AddVertexTN(vertices[ps->vertices[0]].pos, tex->xstart, tex->ystart, ps->normals[0]);
			va2->AddVertexTN(vertices[ps->vertices[1]].pos, tex->xend,   tex->ystart, ps->normals[1]);
			va2->AddVertexTN(vertices[ps->vertices[2]].pos, tex->xend,   tex->yend,   ps->normals[2]);
		} else {
			glNormal3f(ps->normal.x, ps->normal.y, ps->normal.z);
			glBegin(GL_TRIANGLE_FAN);
			glTexCoord2f(tex->xstart, tex->ystart);

			for (std::vector<int>::const_iterator fi = ps->vertices.begin(); fi != ps->vertices.end(); ++fi) {
				const float3& t = vertices[(*fi)].pos;

				glNormalf3(ps->normal);
				glVertex3f(t.x, t.y, t.z);
			}
			glEnd();
		}
	}

	va1->DrawArrayTN(GL_QUADS);

	if (va2->drawIndex() != 0) {
		va2->DrawArrayTN(GL_TRIANGLES);
	}

	// glFrontFace(GL_CCW);
}

void S3DOPiece::SetMinMaxExtends()
{
	for (std::vector<S3DOVertex>::const_iterator vi = vertices.begin(); vi != vertices.end(); ++vi) {
		mins.x = std::min(mins.x, (goffset.x + vi->pos.x));
		mins.y = std::min(mins.y, (goffset.y + vi->pos.y));
		mins.z = std::min(mins.z, (goffset.z + vi->pos.z));

		maxs.x = std::max(maxs.x, (goffset.x + vi->pos.x));
		maxs.y = std::max(maxs.y, (goffset.y + vi->pos.y));
		maxs.z = std::max(maxs.z, (goffset.z + vi->pos.z));
	}
}

void S3DOPiece::Shatter(float pieceChance, int /*texType*/, int team, const float3& pos, const float3& speed) const
{
	for (std::vector<S3DOPrimitive>::const_iterator pi = prims.begin(); pi != prims.end(); ++pi) {
		if (gu->usRandFloat() > pieceChance || pi->numVertex != 4)
			continue;

		ph->AddFlyingPiece(team, pos, speed + gu->usRandVector() * 2.0f, this, &*pi);
	}
}
