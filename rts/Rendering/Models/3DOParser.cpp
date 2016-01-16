/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "3DOParser.h"


#include "Game/GlobalUnsynced.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/VertexArray.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/FlyingPiece.h"
#include "System/Util.h"
#include "System/Exceptions.h"
#include "System/Matrix44f.h"
#include "System/Log/ILog.h"
#include "System/FileSystem/VFSHandler.h"
#include "System/FileSystem/FileHandler.h"
#include "System/FileSystem/SimpleParser.h"
#include "System/Platform/byteorder.h"

#include <list>
#include <vector>
#include <cctype>
#include <boost/cstdint.hpp>

using std::list;
using std::map;


#define SCALE_FACTOR_3DO (1.0f / 65536.0f)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define READ_3DOBJECT(o, fileBuf, curOffset)			\
do {													\
	unsigned int __tmp;									\
	unsigned short __isize = sizeof(unsigned int);		\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(o).VersionSignature = (int)swabDWord(__tmp);		\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(o).NumberOfVertices = (int)swabDWord(__tmp);		\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(o).NumberOfPrimitives = (int)swabDWord(__tmp);		\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(o).SelectionPrimitive = (int)swabDWord(__tmp);		\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(o).XFromParent = (int)swabDWord(__tmp);			\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(o).YFromParent = (int)swabDWord(__tmp);			\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(o).ZFromParent = (int)swabDWord(__tmp);			\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(o).OffsetToObjectName = (int)swabDWord(__tmp);		\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(o).Always_0 = (int)swabDWord(__tmp);				\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(o).OffsetToVertexArray = (int)swabDWord(__tmp);	\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(o).OffsetToPrimitiveArray = (int)swabDWord(__tmp);	\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(o).OffsetToSiblingObject = (int)swabDWord(__tmp);	\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(o).OffsetToChildObject = (int)swabDWord(__tmp);	\
} while (0)


#define READ_VERTEX(v, fileBuf, curOffset)				\
do {													\
	unsigned int __tmp;									\
	unsigned short __isize = sizeof(unsigned int);		\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(v).x = (int)swabDWord(__tmp);						\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(v).y = (int)swabDWord(__tmp);						\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);	\
	(v).z = (int)swabDWord(__tmp);						\
} while (0)


#define READ_PRIMITIVE(p, fileBuf, curOffset)				\
do {														\
	unsigned int __tmp;										\
	unsigned short __isize = sizeof(unsigned int);			\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);		\
	(p).PaletteEntry = (int)swabDWord(__tmp);				\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);		\
	(p).NumberOfVertexIndexes = (int)swabDWord(__tmp);		\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);		\
	(p).Always_0 = (int)swabDWord(__tmp);					\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);		\
	(p).OffsetToVertexIndexArray = (int)swabDWord(__tmp);	\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);		\
	(p).OffsetToTextureName = (int)swabDWord(__tmp);		\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);		\
	(p).Unknown_1 = (int)swabDWord(__tmp);					\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);		\
	(p).Unknown_2 = (int)swabDWord(__tmp);					\
	SimStreamRead(&__tmp,__isize, fileBuf, curOffset);		\
	(p).Unknown_3 = (int)swabDWord(__tmp);					\
} while (0)



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
	int curOffset = 0;

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

	S3DOPiece* rootPiece = LoadPiece(model, 0, NULL, &model->numPieces, fileBuf, curOffset);

	model->SetRootPiece(rootPiece);

	// set after the extrema are known
	model->radius = (model->maxs   - model->mins  ).Length() * 0.5f;
	model->height = (model->maxs.y - model->mins.y);

	model->drawRadius = model->radius;
	model->relMidPos = (model->maxs + model->mins) * 0.5f;

	return model;
}


void C3DOParser::GetVertexes(_3DObject* o, S3DOPiece* object, const std::vector<unsigned char>& fileBuf, int& curOffset)
{
	curOffset = o->OffsetToVertexArray;
	object->vertices.reserve(o->NumberOfVertices);

	for (int a = 0; a < o->NumberOfVertices; a++) {
		float3 v;
		READ_VERTEX(v, fileBuf, curOffset);

		v *= SCALE_FACTOR_3DO;
		v.z = -v.z;

		S3DOVertex vertex;
		vertex.pos = v;

		object->vertices.push_back(vertex);
	}
}


void C3DOParser::GetPrimitives(S3DOPiece* obj, int pos, int num, int excludePrim, const std::vector<unsigned char>& fileBuf, int& curOffset)
{
	map<int,int> prevHashes;

	for(int a=0;a<num;a++){
		if(excludePrim==a){
			continue;
		}
		curOffset=pos+a*sizeof(_Primitive);
		_Primitive p;

		READ_PRIMITIVE(p, fileBuf, curOffset);
		S3DOPrimitive sp;
		sp.numVertex=p.NumberOfVertexIndexes;

		if(sp.numVertex<3)
			continue;

		sp.vertices.reserve(sp.numVertex);
		sp.vnormals.reserve(sp.numVertex);

		curOffset=p.OffsetToVertexIndexArray;
		boost::uint16_t w;

		list<int> orderVert;
		for(int b=0;b<sp.numVertex;b++){
			SimStreamRead(&w,2, fileBuf, curOffset);
			swabWordInPlace(w);
			sp.vertices.push_back(w);
			orderVert.push_back(w);
		}
		orderVert.sort();
		int vertHash=0;

		for(list<int>::iterator vi=orderVert.begin();vi!=orderVert.end();++vi)
			vertHash=(vertHash+(*vi))*(*vi);


		std::string texName;

		if (p.OffsetToTextureName != 0) {
			texName = StringToLower(GetText(p.OffsetToTextureName, fileBuf, curOffset));

			if (teamtex.find(texName) == teamtex.end()) {
				texName += "00";
			}
		} else {
			texName = "ta_color" + IntToString(p.PaletteEntry, "%i");
		}

		if ((sp.texture = texturehandler3DO->Get3DOTexture(texName)) == NULL) {
			LOG_L(L_WARNING, "[%s] unknown 3DO texture \"%s\" for piece \"%s\"",
					__FUNCTION__, texName.c_str(), obj->name.c_str());

			// assign a dummy texture (the entire atlas)
			sp.texture = texturehandler3DO->Get3DOTexture("___dummy___");
		}


		const float3 v0v1 = (obj->vertices[sp.vertices[1]].pos - obj->vertices[sp.vertices[0]].pos);
		const float3 v0v2 = (obj->vertices[sp.vertices[2]].pos - obj->vertices[sp.vertices[0]].pos);

		float3 n = (-v0v1.cross(v0v2)).SafeNormalize();

		// set the primitive-normal and copy it <numVertex>
		// times (vnormals get overwritten by CalcNormals())
		sp.primNormal = n;
		sp.vnormals.insert(sp.vnormals.begin(), sp.numVertex, n);

		// sometimes there are more than one selection primitive (??)
		if (n.dot(-UpVector) > 0.99f) {
			bool ignore=true;

			if(sp.numVertex!=4) {
				ignore=false;
			} else {
				const float3 s1 = obj->vertices[sp.vertices[0]].pos - obj->vertices[sp.vertices[1]].pos;
				const float3 s2 = obj->vertices[sp.vertices[1]].pos - obj->vertices[sp.vertices[2]].pos;

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
		}
		curOffset = p.OffsetToVertexIndexArray;

		for (int b = 0; b < sp.numVertex; b++) {
			SimStreamRead(&w, 2, fileBuf, curOffset);
			swabWordInPlace(w);
			obj->vertices[w].prims.push_back(obj->prims.size() - 1);
		}
	}
}


void C3DOParser::CalcNormals(S3DOPiece* o) const
{
	for (std::vector<S3DOPrimitive>::iterator ps = o->prims.begin(); ps != o->prims.end(); ++ps) {
		S3DOPrimitive* prim = &(*ps);

		for (int a = 0; a < prim->numVertex; ++a) {
			S3DOVertex* vertex = &o->vertices[prim->vertices[a]];
			vertex->normal = ZeroVector;

			// visit all primitives shared by this vertex, Gouraud-style
			for (std::vector<int>::iterator pi = vertex->prims.begin(); pi != vertex->prims.end(); ++pi) {
				const float3& primNormal = o->prims[*pi].primNormal;

				// consider two primitives part of the same surface if
				// angle between their normals is less than ~63 degrees
				if (prim->primNormal.dot(primNormal) > 0.45f) {
					vertex->normal += primNormal;
				}
			}

			// now make the normal for vertex <a> equal the smoothed normal
			prim->vnormals[a] = vertex->normal.SafeNormalize();
		}
	}
}


std::string C3DOParser::GetText(int pos, const std::vector<unsigned char>& fileBuf, int& curOffset)
{
	curOffset = pos;
	char c;
	std::string s;

	SimStreamRead(&c, 1, fileBuf, curOffset);

	while (c != 0) {
		s += c;
		SimStreamRead(&c, 1, fileBuf, curOffset);
	}

	return s;
}


S3DOPiece* C3DOParser::LoadPiece(S3DModel* model, int pos, S3DOPiece* parent, int* numobj, const std::vector<unsigned char>& fileBuf, int& curOffset)
{
	(*numobj)++;

	_3DObject me;

	curOffset = pos;
	READ_3DOBJECT(me, fileBuf, curOffset);

	std::string s = GetText(me.OffsetToObjectName, fileBuf, curOffset);
	StringToLowerInPlace(s);

	S3DOPiece* piece = new S3DOPiece();
		piece->name = s;
		piece->parent = parent;
		piece->offset.x =  me.XFromParent * SCALE_FACTOR_3DO;
		piece->offset.y =  me.YFromParent * SCALE_FACTOR_3DO;
		piece->offset.z = -me.ZFromParent * SCALE_FACTOR_3DO;
		piece->goffset = piece->offset + ((parent != NULL)? parent->goffset: ZeroVector);

	GetVertexes(&me, piece, fileBuf, curOffset);
	GetPrimitives(piece, me.OffsetToPrimitiveArray, me.NumberOfPrimitives, ((pos == 0)? me.SelectionPrimitive: -1), fileBuf, curOffset);
	CalcNormals(piece);
	piece->SetMinMaxExtends();

	model->mins = float3::min(piece->goffset + piece->mins, model->mins);
	model->maxs = float3::max(piece->goffset + piece->maxs, model->maxs);

	piece->SetCollisionVolume(CollisionVolume("box", piece->maxs - piece->mins, (piece->maxs + piece->mins) * 0.5f));

	piece->radius = (piece->GetCollisionVolume())->GetBoundingRadius();
	piece->relMidPos = (piece->GetCollisionVolume())->GetOffsets();

	if (me.OffsetToChildObject > 0) {
		piece->children.push_back(LoadPiece(model, me.OffsetToChildObject, piece, numobj, fileBuf, curOffset));
	}

	piece->SetHasGeometryData(!piece->prims.empty());

	if (me.OffsetToSiblingObject > 0) {
		parent->children.push_back(LoadPiece(model, me.OffsetToSiblingObject, parent, numobj, fileBuf, curOffset));
	}

	return piece;
}



void C3DOParser::SimStreamRead(void* buf, int length, const std::vector<unsigned char>& fileBuf, int& curOffset)
{
	memcpy(buf, &fileBuf[curOffset], length);
	curOffset += length;
}






void S3DOPiece::DrawForList() const
{
	if (!hasGeometryData)
		return;

	// note: do not use more than two VA's
	// via GetVertexArray(), it wraps around
	CVertexArray* va1 = GetVertexArray();
	CVertexArray* va2 = GetVertexArray();
	va1->Initialize();
	va2->Initialize();

	for (std::vector<S3DOPrimitive>::const_iterator ps = prims.begin(); ps != prims.end(); ++ps) {
		C3DOTextureHandler::UnitTexture* tex = ps->texture;

		if (ps->numVertex == 4) {
			va1->AddVertexTN(vertices[ps->vertices[0]].pos, tex->xstart, tex->ystart, ps->vnormals[0]);
			va1->AddVertexTN(vertices[ps->vertices[1]].pos, tex->xend,   tex->ystart, ps->vnormals[1]);
			va1->AddVertexTN(vertices[ps->vertices[2]].pos, tex->xend,   tex->yend,   ps->vnormals[2]);
			va1->AddVertexTN(vertices[ps->vertices[3]].pos, tex->xstart, tex->yend,   ps->vnormals[3]);
		} else if (ps->numVertex == 3) {
			va2->AddVertexTN(vertices[ps->vertices[0]].pos, tex->xstart, tex->ystart, ps->vnormals[0]);
			va2->AddVertexTN(vertices[ps->vertices[1]].pos, tex->xend,   tex->ystart, ps->vnormals[1]);
			va2->AddVertexTN(vertices[ps->vertices[2]].pos, tex->xend,   tex->yend,   ps->vnormals[2]);
		} else {
			/*glBegin(GL_TRIANGLE_FAN);
			glNormalf3(ps->primNormal);
			glTexCoord2f(tex->xstart, tex->ystart);

			for (std::vector<int>::const_iterator fi = ps->vertices.begin(); fi != ps->vertices.end(); ++fi) {
				glVertexf3(vertices[(*fi)].pos);
			}
			glEnd();*/

			//workaround: split fan into triangles to workaround a bug in Mesa drivers with fans, dlists & glbegin..glend
			if (ps->vertices.size() >= 3) {
				std::vector<int>::const_iterator fi = ps->vertices.begin();
				const float3* edge1 = &(vertices[*fi].pos); ++fi;
				const float3* edge2 = &(vertices[*fi].pos); ++fi;
				for (; fi != ps->vertices.end(); ++fi) {
					const float3* edge3 = &(vertices[*fi].pos);
					va2->AddVertexTN(*edge1, tex->xstart, tex->ystart, ps->primNormal);
					va2->AddVertexTN(*edge2, tex->xstart, tex->ystart, ps->primNormal);
					va2->AddVertexTN(*edge3, tex->xstart, tex->ystart, ps->primNormal);
					edge2 = edge3;
				}
			}
		}
	}

	va1->DrawArrayTN(GL_QUADS);

	if (va2->drawIndex() != 0) {
		va2->DrawArrayTN(GL_TRIANGLES);
	}
}

void S3DOPiece::SetMinMaxExtends()
{
	for (auto vi = vertices.cbegin(); vi != vertices.cend(); ++vi) {
		mins = float3::min(mins, vi->pos);
		maxs = float3::max(maxs, vi->pos);
	}
}

void S3DOPiece::Shatter(float pieceChance, int /*texType*/, int team, const float3 pos, const float3 speed, const CMatrix44f& m) const
{
	for (std::vector<S3DOPrimitive>::const_iterator pi = prims.begin(); pi != prims.end(); ++pi) {
		if (gu->RandFloat() > pieceChance || pi->numVertex != 4)
			continue;

		FlyingPiece* fp = new S3DOFlyingPiece(pos, speed, team, this, &*pi);
		projectileHandler->AddFlyingPiece(MODELTYPE_3DO, fp);
	}
}
