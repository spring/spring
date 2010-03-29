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
#include "System/Platform/errorhandler.h"
#include "System/Platform/byteorder.h"
#include <boost/cstdint.hpp>

using namespace std;

static const float  scaleFactor = 1 / (65536.0f);
static const float3 DownVector  = -UpVector;

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

	while(!file.Eof()) {
		teamtex.insert(StringToLower(parser.GetLine()));
	}
}


S3DModel* C3DOParser::Load(const string& name)
{
	CFileHandler file(name);
	if (!file.FileExists()) {
		throw content_error("[3DOParser] could not find model-file " + name);
	}

	fileBuf = new unsigned char[file.FileSize()];
	file.Read(fileBuf, file.FileSize());

	if (fileBuf == NULL) {
		delete[] fileBuf;
		throw content_error("Failed to read file " + name);
	}

	S3DModel* model = new S3DModel;
		model->name = name;
		model->type = MODELTYPE_3DO;
		model->textureType = 0;
		model->numobjects  = 0;

	// Load the Model
	S3DOPiece* rootobj = ReadChild(0, NULL, &model->numobjects);
	model->rootobject = rootobj;

	// PreProcessing
	FindCenter(rootobj);

	rootobj->radius = FindRadius(rootobj, -rootobj->relMidPos);

	rootobj->relMidPos.x = 0.0f; // ?
	rootobj->relMidPos.z = 0.0f; // ?
	rootobj->relMidPos.y = std::max(rootobj->relMidPos.y, 1.0f); // ?

	model->radius = rootobj->radius;
	model->height = FindHeight(rootobj, ZeroVector);

	model->maxs = rootobj->maxs;
	model->mins = rootobj->mins;

	model->relMidPos = rootobj->relMidPos;

	delete[] fileBuf;
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


void C3DOParser::GetPrimitives(S3DOPiece* obj,int pos,int num,vertex_vector* vv,int excludePrim)
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
		float3 n=-(obj->vertices[sp.vertices[1]].pos-obj->vertices[sp.vertices[0]].pos).cross(obj->vertices[sp.vertices[2]].pos-obj->vertices[sp.vertices[0]].pos);
		n.SafeNormalize();
		sp.normal=n;
		sp.normals.insert(sp.normals.begin(), sp.numVertex, n);

		//sometimes there are more than one selection primitive (??)
		if(n.dot(DownVector)>0.99f){
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
			obj->isEmpty=false;
		}
		curOffset=p.OffsetToVertexIndexArray;

		for(int b=0;b<sp.numVertex;b++){
			SimStreamRead(&w,2);
			w = swabword(w);
			obj->vertices[w].prims.push_back(obj->prims.size()-1);
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


S3DOPiece* C3DOParser::ReadChild(int pos, S3DOPiece* root, int* numobj)
{
	(*numobj)++;

	S3DOPiece* object = new S3DOPiece;
	_3DObject me;

	curOffset = pos;
	READ_3DOBJECT(me);

	std::string s = GetText(me.OffsetToObjectName);
	StringToLowerInPlace(s);
	object->name = s;
	object->displist = 0;
	object->type = MODELTYPE_3DO;

	object->offset.x = me.XFromParent*scaleFactor;
	object->offset.y = me.YFromParent*scaleFactor;
	object->offset.z =-me.ZFromParent*scaleFactor;
	std::vector<float3> vertexes;

	GetVertexes(&me, object);
	GetPrimitives(object, me.OffsetToPrimitiveArray, me.NumberOfPrimitives, &vertexes, (pos == 0? me.SelectionPrimitive: -1));
	CalcNormals(object);

	if (me.OffsetToChildObject > 0) {
		object->childs.push_back( ReadChild(me.OffsetToChildObject, object, numobj) );
	}

	object->vertexCount = object->vertices.size();
	object->isEmpty = (object->prims.size() < 1);

	if (me.OffsetToSiblingObject > 0) {
		root->childs.push_back( ReadChild(me.OffsetToSiblingObject, root, numobj) );
	}

	return object;
}



void C3DOParser::SimStreamRead(void* buf, int length)
{
	memcpy(buf, &fileBuf[curOffset], length);
	curOffset += length;
}


void C3DOParser::FindCenter(S3DOPiece* o) const
{
	std::vector<S3DModelPiece*>::iterator si;
	for (si = o->childs.begin(); si != o->childs.end(); ++si) {
		FindCenter((S3DOPiece*) *si);
	}

	float maxSize = 0;
	float maxx = -1000.0f, maxy = -1000.0f, maxz = -1000.0f;
	float minx = 10000.0f, miny = 10000.0f, minz = 10000.0f;

	std::vector<S3DOVertex>::iterator vi;
	for (vi = o->vertices.begin(); vi != o->vertices.end(); ++vi) {
		maxx = max(maxx, vi->pos.x);
		maxy = max(maxy, vi->pos.y);
		maxz = max(maxz, vi->pos.z);

		minx = min(minx, vi->pos.x);
		miny = min(miny, vi->pos.y);
		minz = min(minz, vi->pos.z);
	}
	for (si = o->childs.begin(); si != o->childs.end(); ++si) {
		maxx = max(maxx, (*si)->offset.x + (*si)->maxs.x);
		maxy = max(maxy, (*si)->offset.y + (*si)->maxs.y);
		maxz = max(maxz, (*si)->offset.z + (*si)->maxs.z);

		minx = min(minx, (*si)->offset.x + (*si)->mins.x);
		miny = min(miny, (*si)->offset.y + (*si)->mins.y);
		minz = min(minz, (*si)->offset.z + (*si)->mins.z);
	}

	o->maxs = float3(maxx, maxy, maxz);
	o->mins = float3(minx, miny, minz);

	const float3 cvScales((o->maxs.x - o->mins.x),        (o->maxs.y - o->mins.y),        (o->maxs.z - o->mins.z)       );
	const float3 cvOffset((o->maxs.x + o->mins.x) * 0.5f, (o->maxs.y + o->mins.y) * 0.5f, (o->maxs.z + o->mins.z) * 0.5f);

	o->colvol = new CollisionVolume("box", cvScales, cvOffset, COLVOL_TEST_CONT);
	o->colvol->Enable();

	o->relMidPos = cvOffset;

	for (vi = o->vertices.begin(); vi != o->vertices.end(); ++vi) {
		maxSize = max(maxSize, o->relMidPos.distance(vi->pos));
	}
	for (si = o->childs.begin(); si != o->childs.end(); ++si) {
		S3DOPiece* p3do = (S3DOPiece*) (*si);
		maxSize = max(maxSize, o->relMidPos.distance(p3do->offset + p3do->relMidPos) + p3do->radius);
	}
	o->radius = maxSize;
}


float C3DOParser::FindRadius(const S3DOPiece* object, float3 offset) const
{
	float maxSize = 0.0f;
	offset += object->offset;

	std::vector<S3DModelPiece*>::const_iterator si;
	for (si = object->childs.begin(); si != object->childs.end(); ++si) {
		float maxChild = FindRadius((S3DOPiece*) *si, offset);

		if (maxChild > maxSize) {
			maxSize = maxChild;
		}
	}

	std::vector<S3DOVertex>::const_iterator vi;
	for (vi = object->vertices.begin(); vi != object->vertices.end(); ++vi) {
		maxSize = max(maxSize, (vi->pos + offset).Length());
	}

	return (maxSize * 0.8f);
}


float C3DOParser::FindHeight(const S3DOPiece* object, float3 offset) const
{
	float height = 0.0;
	offset += object->offset;

	std::vector<S3DModelPiece*>::const_iterator si;
	for (si = object->childs.begin(); si != object->childs.end(); ++si) {
		float maxChild = FindHeight((S3DOPiece*) *si, offset);

		if (maxChild > height) {
			height = maxChild;
		}
	}

	std::vector<S3DOVertex>::const_iterator vi;
	for (vi = object->vertices.begin(); vi != object->vertices.end(); ++vi) {
		if (vi->pos.y + offset.y > height) {
			height = vi->pos.y + offset.y;
		}
	}

	return height;
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

void S3DOPiece::Shatter(float pieceChance, int /*texType*/, int team, const float3& pos, const float3& speed) const
{
	for (std::vector<S3DOPrimitive>::const_iterator pi = prims.begin(); pi != prims.end(); ++pi) {
		if (gu->usRandFloat() > pieceChance || pi->numVertex != 4)
			continue;

		ph->AddFlyingPiece(team, pos, speed + gu->usRandVector() * 2.0f, this, &*pi);
	}
}
